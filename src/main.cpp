#include <chrono>
#include <windows.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <thread>
#include <dxgi.h>
#include <libmem.h>
#include <imgui.h>
#include <winuser.h>
#include <xlocale>
#include "include/api.h"


#include "include/layer.h"
#include "include/menu.hpp"
#include "include/mod_loader.h"
#include <tlhelp32.h>


HMODULE dllHandle = nullptr;

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
    AllocConsole();
    SetConsoleTitle("sml console");

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
 
/*

    TCHAR dllPath[1024 + 64];
    DWORD dllPathSize = GetSystemDirectory(dllPath, 1024);
    if (dllPathSize == 0)
    {
        Fail("Could not get system directory path");
    }

    
    wcscpy_s(dllPath + dllPathSize, std::size(dllPath) - dllPathSize, L"\\powrprof.dll");
*/


    dllHandle = LoadLibrary("C:\\Windows\\System32\\powrprof.dll");
    if (dllHandle == NULL) {
        dllHandle = LoadLibrary("C:\\Windows\\System32\\POWRPROF.dll");
        if(dllHandle == NULL){
            printf("failed to load POWRPROF.dll");
        }
    }

    o_GetPwrCapabilities = (BOOLEAN(*)(PSYSTEM_POWER_CAPABILITIES))GetProcAddress(dllHandle, "GetPwrCapabilities");
    o_CallNtPowerInformation = (NTSTATUS (*)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG))GetProcAddress(dllHandle, "CallNtPowerInformation");
    o_PowerDeterminePlatformRole = (POWER_PLATFORM_ROLE (*)())GetProcAddress(dllHandle, "PowerDeterminePlatformRole");
    
    if (o_GetPwrCapabilities == nullptr || o_CallNtPowerInformation == nullptr || o_PowerDeterminePlatformRole == nullptr) {
        printf("Could not locate symbols in powrprof.dll");
    }

}





DWORD WINAPI DllThread(LPVOID lpParam){
	InitConsole();
    loadWrapper();
    ModApi::Instance().InitSkyBase();

    //HOOK_SET(ModApi::Instance().GetSkyBase() + 0x13311C0, AvatarEnergy_Use);
    ModLoader::LoadMods();
    //uintptr_t CheckpointBarn = *(uintptr_t *)(gv::gamePtr + 0x1);
    //CheckpointBarn_m_ChangeLevel = (uintptr_t (*)(uintptr_t, uintptr_t, const char *))(ModApi::Instance().GetSkyBase() + 0x1188810);


    return EXIT_SUCCESS;
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




DWORD WINAPI hook_thread(PVOID lParam){
    HWND window;
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring ws(path);
    std::string _path(ws.begin(), ws.end());
    SetEnvironmentVariable("VK_ADD_LAYER_PATH", _path.substr(0, _path.find_last_of("\\/")).c_str());
    SetEnvironmentVariable("VK_LOADER_LAYERS_ENABLE", "VkLayer_lukas_sml,*validation"); 
        		
    while(!window){
        printf("searching for window \n");
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); //prone for a race condition (1 second crashes);
        window = FindWindowA("TgcMainWindow", "Sky");
        } 
    printf("found window: %p\n", window);
    layer::setup(window);      
    oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
    return EXIT_SUCCESS;
}



void onAttach(){
    terminateCrashpadHandler();
    static HANDLE dllHandle = CreateThread(NULL, 0, DllThread, NULL, 0, NULL);
    static HANDLE hook = CreateThread(NULL, 0, hook_thread, nullptr, 0, NULL);
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
