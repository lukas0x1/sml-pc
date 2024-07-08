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
#include "include/api.h"

#include "include/layer.h"
#include "include/menu.hpp"
#include "include/mod_loader.h"
#include <tlhelp32.h>


HMODULE dllHandle = nullptr;
std::string g_path;


BOOLEAN (*o_GetPwrCapabilities)(PSYSTEM_POWER_CAPABILITIES);
NTSTATUS (*o_CallNtPowerInformation)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG);
POWER_PLATFORM_ROLE (*o_PowerDeterminePlatformRole)();

extern "C"
__declspec(dllexport) BOOLEAN __stdcall GetPwrCapabilities(PSYSTEM_POWER_CAPABILITIES lpspc) {
    return o_GetPwrCapabilities(lpspc); 
}

extern "C"
__declspec(dllexport) NTSTATUS __stdcall CallNtPowerInformation(POWER_INFORMATION_LEVEL InformationLevel, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength){
    return o_CallNtPowerInformation(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

extern "C"
__declspec(dllexport) POWER_PLATFORM_ROLE PowerDeterminePlatformRole(){
    return o_PowerDeterminePlatformRole();
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

    FILE * outputStream;
    freopen_s(&outputStream, "CONOUT$", "w", stdout);
    FILE* inputStream;
    freopen_s(&inputStream, "CONIN$", "r", stdin);
}


void loadWrapper(){
    dllHandle = LoadLibrary("C:\\Windows\\System32\\powrprof.dll");
    if (dllHandle == NULL) {
        dllHandle = LoadLibrary("C:\\Windows\\System32\\POWRPROF.dll");
    }

    if (dllHandle != NULL) {
        o_GetPwrCapabilities = (BOOLEAN(*)(PSYSTEM_POWER_CAPABILITIES))GetProcAddress(dllHandle, "GetPwrCapabilities");
        o_CallNtPowerInformation = (NTSTATUS (*)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG))GetProcAddress(dllHandle, "CallNtPowerInformation");
        o_PowerDeterminePlatformRole = (POWER_PLATFORM_ROLE (*)())GetProcAddress(dllHandle, "PowerDeterminePlatformRole");
    
    if (o_GetPwrCapabilities == nullptr || o_CallNtPowerInformation == nullptr || o_PowerDeterminePlatformRole == nullptr) {
        printf("Could not locate symbols in powrprof.dll");
    }
    }
    else {
        printf("failed to load POWRPROF.dll");
    }
}


static WNDPROC oWndProc;
static LRESULT WINAPI WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_HOME) {
            Menu::bShowMenu = !Menu::bShowMenu;
            return 0;
        }
    }
    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if(Menu::bShowMenu) {
        if(ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            return ERROR_SUCCESS;
        }
        if (ImGui::GetIO().WantCaptureMouse && (uMsg == WM_LBUTTONDOWN || 
                uMsg == WM_LBUTTONUP || uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP || 
                uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP || uMsg == WM_MOUSEWHEEL || 
                uMsg == WM_MOUSEMOVE))
            return ERROR_SUCCESS;
    }
    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


void terminateCrashpadHandler() {

    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry) == TRUE) {
        while (Process32Next(snapshot, &entry) == TRUE) {
            if (lstrcmp(entry.szExeFile, "crashpad_handler.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);

                if (hProcess!= NULL) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
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

void clear_screen(char fill = ' ') { 
    COORD tl = {0,0};
    CONSOLE_SCREEN_BUFFER_INFO s;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);   
    GetConsoleScreenBufferInfo(console, &s);
    DWORD written, cells = s.dwSize.X * s.dwSize.Y;
    FillConsoleOutputCharacter(console, fill, cells, tl, &written);
    FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
    SetConsoleCursorPosition(console, tl);
}


typedef LSTATUS (__stdcall *PFN_RegEnumValueA)(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
PFN_RegEnumValueA oRegEnumValueA;
LSTATUS hkRegEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData){

    std::wstring path = GetKeyPathFromKKEY(hKey);
    std::string name = g_path + "\\sml_config.json";
    
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


DWORD WINAPI hook_thread(PVOID lParam){
    HWND window = nullptr; 
    printf("Searching for window...\n");
    while(!window){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        window = FindWindowA("TgcMainWindow", "Sky");
    } 
    printf("Window Found: %p\n", window);
    layer::setup(window);      
    oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
    InitConsole();
    Sleep(3000);
    //clear_screen();
    return EXIT_SUCCESS;
}

DWORD WINAPI console_thread(LPVOID lpParam){
    
    return EXIT_SUCCESS;
}


void onAttach(){
    loadWrapper();
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    std::string _path(ws.begin(), ws.end());
    g_path = _path.substr(0, _path.find_last_of("\\/"));
    HMODULE handle = LoadLibrary("advapi32.dll");
    if(handle!= NULL){
        lm_address_t fnRegEnumValue = (lm_address_t)GetProcAddress(handle, "RegEnumValueA");
        LM_HookCode(fnRegEnumValue, (lm_address_t)&hkRegEnumValueA, (lm_address_t *)&oRegEnumValueA);
        InitConsole();
        terminateCrashpadHandler();
        ModApi::Instance().InitSkyBase();
        ModLoader::LoadMods();
        CreateThread(NULL, 0, console_thread, NULL, 0, NULL);
        CreateThread(NULL, 0, hook_thread, nullptr, 0, NULL);
    }
    else {
        printf("Failed to load advapi32.dll");
    }
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){
    DisableThreadLibraryCalls(hinstDLL);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            onAttach();

            break;
        case DLL_PROCESS_DETACH:
        
            break;
    }
    
    return TRUE;

}