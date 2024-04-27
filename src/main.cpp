#include <windows.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <thread>
#include <dxgi.h>
#include <libmem.h>
#include <imgui.h>
#include "include/api.h"
#include "include/vulkan_hooks.hpp"
#include "include/opengl_hooks.hpp"
#include "include/dx11_hooks.hpp"
#include "include/menu.hpp"
#include "include/mod_loader.h"


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


// uintptr_t (*CheckpointBarn_m_ChangeLevel)(uintptr_t CheckpointBarn, uintptr_t Game, const char *level);
// HOOK_DEF(uintptr_t, AvatarEnergy_Use, uintptr_t instance, unsigned int EnergyRule, float energy) {

//     CheckpointBarn_m_ChangeLevel(NULL, gv::gamePtr, "CandleSpaceEnd");
//     return orig_AvatarEnergy_Use(instance, EnergyRule, -3.0);
//     //return EnergyRule;
// }




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
    loadWrapper();
    ModApi::Instance().InitSkyBase();

    //HOOK_SET(ModApi::Instance().GetSkyBase() + 0x13311C0, AvatarEnergy_Use);
    ModLoader::LoadMods();
    //uintptr_t CheckpointBarn = *(uintptr_t *)(gv::gamePtr + 0x1);
    //CheckpointBarn_m_ChangeLevel = (uintptr_t (*)(uintptr_t, uintptr_t, const char *))(ModApi::Instance().GetSkyBase() + 0x1188810);

    return EXIT_SUCCESS;
}



static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam) {
    const auto isMainWindow = [ handle ]( ) {
        return GetWindow(handle, GW_OWNER) == nullptr && IsWindowVisible(handle);
    };

    DWORD pID = 0;
    GetWindowThreadProcessId(handle, &pID);

    if (GetCurrentProcessId( ) != pID || !isMainWindow( ) || handle == GetConsoleWindow( ))
        return TRUE;

    *reinterpret_cast<HWND*>(lParam) = handle;

    return FALSE;
}

HWND GetProcessWindow( ) {
    HWND hwnd = nullptr;
    EnumWindows(::EnumWindowsCallback, reinterpret_cast<LPARAM>(&hwnd));

    while (!hwnd) {
        EnumWindows(::EnumWindowsCallback, reinterpret_cast<LPARAM>(&hwnd));
        printf("[!] Waiting for window to appear.\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    char name[128];
    GetWindowTextA(hwnd, name, RTL_NUMBER_OF(name));
    printf("[+] Got window with name: '%s'\n", name);

    return hwnd;
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



DWORD WINAPI hook_thread(PVOID lParam){
    HWND window = GetProcessWindow();
    //VK::Hook(window);
    //GL::Hook(window);
    DX11::Hook(window);

    oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
    return EXIT_SUCCESS;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){


    switch (fdwReason) {
        DisableThreadLibraryCalls(hinstDLL);
        case DLL_PROCESS_ATTACH:
            static HANDLE dllHandle = CreateThread(NULL, 0, DllThread, NULL, 0, NULL);
            static HANDLE hook = CreateThread(NULL, 0, hook_thread, NULL, 0, NULL);
            AllocConsole();
            freopen("CONOUT$", "w", stdout);
            break;
        case DLL_PROCESS_DETACH:
        
            break;
    }
    
    return TRUE;

}
