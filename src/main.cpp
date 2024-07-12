#include <chrono>
#include <corecrt_wstring.h>
#include <string>
#include <windows.h>
#include <cstdio>
#include <vulkan/vulkan.h>
#include <thread>
#include <dxgi.h>
#include <libmem.h>
#include <imgui.h>
#include <winreg.h>
#include <winuser.h>
#include <xlocale>
#include "include/api.h"

#include "include/layer.h"
#include "include/menu.hpp"
#include "include/mod_loader.h"
#include <tlhelp32.h>
#include <minhook/MinHook.h>
#include "include/hooks.hpp"
#include "include/utils.hpp"

std::string g_path;

// Initializes the console window
void InitConsole() {
    FreeConsole();
    AllocConsole();
    SetConsoleTitle("SML Console");

    if (IsValidCodePage(CP_UTF8)) {
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }

    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(hStdout, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleCtrlHandler(NULL, TRUE); // Disable Ctrl+C handling

    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    GetCurrentConsoleFontEx(hStdout, FALSE, &cfi);

    // Change to a more readable font if the user has one of the default eyesore fonts
    if (wcscmp(cfi.FaceName, L"Terminal") == 0 || wcscmp(cfi.FaceName, L"Courier New") == 0 || (cfi.FontFamily & TMPF_VECTOR) == 0) {
        cfi.nFont = 0;
        cfi.dwFontSize.X = 0;
        cfi.dwFontSize.Y = 14;
        cfi.FontFamily = FF_MODERN | TMPF_VECTOR | TMPF_TRUETYPE;
        cfi.FontWeight = FW_NORMAL;
        wcscpy_s(cfi.FaceName, L"Lucida Console");
        SetCurrentConsoleFontEx(hStdout, FALSE, &cfi);
    }

    freopen_s(reinterpret_cast <FILE**> (stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast <FILE**> (stdin), "CONIN$", "r", stdin);
}

// Terminates the crashpad handler process
void TerminateCrashpadHandler() {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            if (lstrcmp(entry.szExeFile, "crashpad_handler.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        }
    }
    CloseHandle(snapshot);
}

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS) 0x00000000L)
#endif

#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS) 0xC0000023L)
#endif

// Gets the path of a registry key
std::wstring GetKeyPathFromKKEY(HKEY key) {
    std::wstring keyPath;
    if (key != NULL) {
        HMODULE dll = LoadLibrary("ntdll.dll");
        if (dll != NULL) {
            typedef DWORD(__stdcall* NtQueryKeyType)(
                HANDLE KeyHandle,
                int KeyInformationClass,
                PVOID KeyInformation,
                ULONG Length,
                PULONG ResultLength);

            NtQueryKeyType func = reinterpret_cast <NtQueryKeyType> (::GetProcAddress(dll, "NtQueryKey"));

            if (func != NULL) {
                DWORD size = 0;
                DWORD result = 0;
                result = func(key, 3, 0, 0, &size);
                if (result == STATUS_BUFFER_TOO_SMALL) {
                    size = size + 2;
                    wchar_t* buffer = new(std::nothrow) wchar_t[size / sizeof(wchar_t)]; // size is in bytes
                    if (buffer != NULL) {
                        result = func(key, 3, buffer, size, &size);
                        if (result == STATUS_SUCCESS) {
                            buffer[size / sizeof(wchar_t)] = L'\0';
                            keyPath = std::wstring(buffer + 2);
                        }

                        delete[] buffer;
                    }
                }
            }

            FreeLibrary(dll);
        }
    }
    return keyPath;
}

#undef STATUS_BUFFER_TOO_SMALL
#undef STATUS_SUCCESS

// Clears the console screen
void ClearScreen(char fill = ' ') {
    COORD tl = {
       0,
       0
    };
    CONSOLE_SCREEN_BUFFER_INFO s;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(console, &s);
    DWORD written, cells = s.dwSize.X * s.dwSize.Y;
    FillConsoleOutputCharacter(console, fill, cells, tl, &written);
    FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
    SetConsoleCursorPosition(console, tl);
}

// Hook for RegEnumValueA
typedef LSTATUS(__stdcall* PFN_RegEnumValueA)(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
PFN_RegEnumValueA oRegEnumValueA;
LSTATUS hkRegEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {

    std::wstring path = GetKeyPathFromKKEY(hKey);
    std::string name = g_path + "\\sml_config.json";

    LSTATUS result = oRegEnumValueA(hKey, dwIndex, lpValueName, lpcchValueName, lpReserved, lpType, lpData, lpcbData);
    if (wcscmp(path.c_str(), L"\\REGISTRY\\MACHINE\\SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers") == 0 && dwIndex == 0) {

        for (size_t i = 0; i < name.size(); i++) {
            lpValueName[i] = name[i];
        }
        lpValueName[name.size()] = '\0';

        *lpcchValueName = 2048; // Max Path Length
        lpData = nullptr;
        *lpcbData = 4;
    }
    return result;
}

// Hook thread
DWORD WINAPI HookThread(PVOID lParam) {
    HWND window = nullptr;
    printf("Searching for window...\n");
    while (!window) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        window = FindWindowA("TgcMainWindow", "Sky");
    }
    printf("Window Found: %p\n", window);
    MH_Initialize();
    H::Init();
    InitConsole();
    Sleep(3000);
    ClearScreen();
    return EXIT_SUCCESS;
}

// Console thread
DWORD WINAPI ConsoleThread(LPVOID lpParam) {
    return EXIT_SUCCESS;
}

// Attaches the DLL
void OnAttach() {
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    std::string _path(ws.begin(), ws.end());
    g_path = _path.substr(0, _path.find_last_of("\\/"));

    HMODULE handle = LoadLibrary("advapi32.dll");
    if (handle != NULL) {
        auto fnRegEnumValue = reinterpret_cast <lm_address_t> (GetProcAddress(handle, "RegEnumValueA"));
        LM_HookCode(fnRegEnumValue, reinterpret_cast <lm_address_t> (&hkRegEnumValueA), reinterpret_cast <lm_address_t*> (&oRegEnumValueA));
        InitConsole();
        TerminateCrashpadHandler();
        ModApi::Instance().InitSkyBase();
        ModLoader::LoadMods();
        CreateThread(NULL, 0, ConsoleThread, NULL, 0, NULL);
        CreateThread(NULL, 0, HookThread, nullptr, 0, NULL);
    }
    else {
        printf("Failed to load advapi32.dll");
    }
}

// Detaches the DLL
DWORD WINAPI OnProcessDetach(LPVOID lpParam) {
    H::Free();
    MH_Uninitialize();
    return 0;
}

// DLL entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    DisableThreadLibraryCalls(hinstDLL);

    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        OnAttach();
        U::SetRenderingBackend(VULKAN);
        break;
    case DLL_PROCESS_DETACH:
        OnProcessDetach(NULL);
        break;
    }

    return TRUE;
}