#include <windows.h>
#include <stdio.h>
#include "include/libmem/libmem.h"
#include <vulkan/vulkan.h>
#include "vulkan_hooks.hpp"
#include <thread>
#include <dxgi.h>
#include "menu.hpp"

uintptr_t sky = NULL;
uintptr_t Game = 0;

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


uintptr_t (*CheckpointBarn_m_ChangeLevel)(uintptr_t CheckpointBarn, uintptr_t Game, const char *level);


uintptr_t (*o_AvatarEnergy_Use)(uintptr_t, unsigned int, float);
uintptr_t h_AvatarEnergy_Use(uintptr_t instance, unsigned int EnergyRule, float energy){


	CheckpointBarn_m_ChangeLevel(NULL, Game, "CandleSpaceEnd");

	return o_AvatarEnergy_Use(instance, EnergyRule, -3.0);
	//return EnergyRule;
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
	loadWrapper();


	while (GetModuleHandle("Sky.exe") == 0)
	{
		Sleep(100);
	}


	
	sky = (uintptr_t)LoadLibrary(TEXT("Sky.exe"));

	LM_HookCode(sky + 0x13311C0, (lm_address_t)h_AvatarEnergy_Use, (lm_address_t *)&o_AvatarEnergy_Use);

	
	uintptr_t Account = 0;
	while(Game == NULL){
		Sleep(100);
		Game = *(uintptr_t *)(sky + 0x23B2FB0);
	}


	uintptr_t CheckpointBarn = *(uintptr_t *)(Game + 0x1);
	CheckpointBarn_m_ChangeLevel = (uintptr_t (*)(uintptr_t, uintptr_t, const char *))(sky + 0x1188810);

	//*(uintptr_t *)(Game + 0xf64);
	//int *AccountType = (int *)(*(uintptr_t *)(Game + 0xf64) + 0x1D8);

	while(Account == NULL){
		Sleep(100);
		Account = *(uintptr_t *)(Game + 0x1D8);
	}

	int *AccountType = (int *)(Account + 0xf64);
	*AccountType = 0;
	while(*AccountType != 9){
		Sleep(1000);
	}

	*AccountType = 0;
	


	
	
	
	//Game = *(uintptr_t **)(sky + 0x23B2FB0);

	//int *AccountType = (int *)(*(uintptr_t *)(Game + 0xf64) + 0x1D8);
	//*AccountType = 0;

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
        if (wParam == VK_INSERT) {
            Menu::bShowMenu = !Menu::bShowMenu;
            return 0;
        }    
    }
    LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (Menu::bShowMenu) {
        ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

        // (Doesn't work for some games like 'Sid Meier's Civilization VI')
        // Window may not maximize from taskbar because 'H::bShowDemoWindow' is set to true by default. ('hooks.hpp')
        //
        // return ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam) == 0;
    }

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}



DWORD WINAPI hook_thread(PVOID lParam){
	HWND window = GetProcessWindow();
	VK::Hook(window);


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
