#include <chrono>

#include <corecrt_wstring.h>
#include <string>
#include <windows.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <thread>
#include <dxgi.h>
#include <libmem.h>
#include <imgui.h>
#include <winreg.h>
#include <winuser.h>
#include <xlocale>
#include <iostream>
#include <fstream>
#include <streambuf>
#include "include/api.h"

#include "include/layer.h"
#include "include/menu.hpp"
#include "include/mod_loader.h"
#include <tlhelp32.h>

//Injection imports
#include <minhook/MinHook.h>
#include "include/hooks.hpp"
#include "include/utils.hpp"

std::string dll_path;

// SML Log file
std::ofstream SML_Log;

// Custom Streambuf for logging to both file and console
class StreamBuf : public std::streambuf {
public:
    StreamBuf(std::streambuf* buf, const std::string& prefix)
        : originalBuf(buf), logPrefix(prefix) {}

protected:
    virtual int overflow(int c) override {
        if (c != EOF) {
            if (c == '\n') {
                SML_Log << logPrefix << buffer << std::endl;
                SML_Log.flush();
                buffer.clear();
            } else {
                buffer += static_cast<char>(c);
            }
        }
        return originalBuf->sputc(c);
    }

private:
    std::streambuf* originalBuf;
    std::string logPrefix;
    std::string buffer;
};

void print(const char* format, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    std::cout << std::string(buffer);
}

void InitLogger(){
    SML_Log.open("SML.log", std::ios::out | std::ios::app);

    // Redirect std::cout, and std::cerr to SML.log
    static StreamBuf coutBuf(std::cout.rdbuf(), "[output] ");
    std::cout.rdbuf(&coutBuf);

    static StreamBuf cerrBuf(std::cerr.rdbuf(), "[error] ");
    std::cerr.rdbuf(&cerrBuf);
}

void InitConsole(){
    FreeConsole();
    AllocConsole();
    SetConsoleTitle("SML Console");

    if (IsValidCodePage(CP_UTF8)) {
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }

    auto hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(hStdout, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    // Disable Ctrl+C handling
    SetConsoleCtrlHandler(NULL, TRUE);

    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    GetCurrentConsoleFontEx(hStdout, FALSE, &cfi);

    // Change to a more readable font if user has one of the default eyesore fonts
    if (wcscmp(cfi.FaceName, L"Terminal") == 0 || wcscmp(cfi.FaceName, L"Courier New") || (cfi.FontFamily & TMPF_VECTOR) == 0) {
        cfi.cbSize = sizeof(cfi);
        cfi.nFont = 0;
        cfi.dwFontSize.X = 0;
        cfi.dwFontSize.Y = 14;
        cfi.FontFamily = FF_MODERN | TMPF_VECTOR | TMPF_TRUETYPE;
        cfi.FontWeight = FW_NORMAL;
        wcscpy_s(cfi.FaceName, L"Lucida Console");
        SetCurrentConsoleFontEx(hStdout, FALSE, &cfi);
    }

    FILE* file;
    freopen_s(&file, "CONOUT$", "w", stdout);
    freopen_s(&file, "CONOUT$", "w", stderr);
    freopen_s(&file, "CONIN$", "r", stdin);

    fflush(stdout);
    fflush(stderr);
}

void terminateCrashpadHandler() {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE) {
        while (Process32Next(snapshot, &entry) == TRUE) {
            if (lstrcmp(entry.szExeFile, "crashpad_handler.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);

                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    std::cout << "Terminated Crashpad Handler\n";
                }
            }
        }
    }
    CloseHandle(snapshot);
}

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#endif

std::wstring GetKeyPathFromKKEY(HKEY key)
{
    std::wstring keyPath;
    if (key != NULL)
    {
        HMODULE dll = LoadLibrary("ntdll.dll");
        if (dll != NULL) {
            typedef DWORD (__stdcall *NtQueryKeyType)(
                HANDLE  KeyHandle,
                int KeyInformationClass,
                PVOID  KeyInformation,
                ULONG  Length,
                PULONG  ResultLength);

            NtQueryKeyType func = reinterpret_cast<NtQueryKeyType>(::GetProcAddress(dll, "NtQueryKey"));

            if (func != NULL) {
                DWORD size = 0;
                DWORD result = 0;
                result = func(key, 3, 0, 0, &size);
                if (result == STATUS_BUFFER_TOO_SMALL)
                {
                    size = size + 2;
                    wchar_t* buffer = new (std::nothrow) wchar_t[size/sizeof(wchar_t)]; // size is in bytes
                    if (buffer != NULL)
                    {
                        result = func(key, 3, buffer, size, &size);
                        if (result == STATUS_SUCCESS)
                        {
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


typedef LSTATUS (__stdcall *PFN_RegEnumValueA)(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
PFN_RegEnumValueA oRegEnumValueA;
LSTATUS hkRegEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData){

    std::wstring path = GetKeyPathFromKKEY(hKey);
    std::string name = dll_path + "\\sml_config.json";
    
    LSTATUS result = oRegEnumValueA(hKey, dwIndex, lpValueName,lpcchValueName ,lpReserved, lpType, lpData, lpcbData);
    if(wcscmp(path.c_str(), L"\\REGISTRY\\MACHINE\\SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers") == 0 && dwIndex == 0 ){

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


DWORD WINAPI hook_thread(PVOID lParam) {
    HWND window = nullptr;
	std::cout << "Waiting for window..." << std::endl;
    while (!window) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        window = FindWindowA("TgcMainWindow", "Sky");
    }
	std::cout << "Sky window found" << std::endl;
	MH_Initialize();
	H::Init();
    return EXIT_SUCCESS;
}

void InitDllPath(HINSTANCE hinstDLL) {
    WCHAR path[MAX_PATH];
    DWORD result = GetModuleFileNameW(hinstDLL, path, MAX_PATH);

    if (result == 0) {
        std::cerr << "Error retrieving module file name: " << GetLastError() << std::endl;
        return;
    }

    std::wstring ws(path);
    std::string fullPath(ws.begin(), ws.end());
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos) {
        dll_path = fullPath.substr(0, pos);
		Menu::setDllPath(dll_path);
    }
    else {
        std::cerr << "Error parsing path: " << fullPath << std::endl;
    }
    std::cout << "DLL Path: " << dll_path << std::endl;
}

void onAttach(HINSTANCE hinstallDLL) {
    InitConsole();
    InitLogger();
	InitDllPath(hinstallDLL);
    WCHAR path[MAX_PATH];
    HMODULE handle = LoadLibrary("advapi32.dll");
    if (handle != NULL) {
        lm_address_t fnRegEnumValue = (lm_address_t)GetProcAddress(handle, "RegEnumValueA");
        LM_HookCode(fnRegEnumValue, (lm_address_t)&hkRegEnumValueA, (lm_address_t*)&oRegEnumValueA);
        terminateCrashpadHandler();
        ModApi::Instance().InitSkyBase();
        ModLoader::LoadMods(dll_path);
        CreateThread(NULL, 0, hook_thread, nullptr, 0, NULL);
    }
    else {
        print("Failed to load advapi32.dll");
    }
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
    DisableThreadLibraryCalls(hinstDLL);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            onAttach(hinstDLL);
            U::SetRenderingBackend(VULKAN);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    
    return TRUE;

}