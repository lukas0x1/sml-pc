#include <windows.h>
#include <stdio.h>
#include "include/libmem/libmem.h"




HMODULE dllHandle = nullptr;
BOOLEAN (*OriginalGetPwrCapabilities)(PSYSTEM_POWER_CAPABILITIES);
NTSTATUS (*OriginalCallNtPowerInformation)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG);
POWER_PLATFORM_ROLE (*OriginalPowerDeterminePlatformRole)();

extern "C"
__declspec(dllexport) BOOLEAN __stdcall GetPwrCapabilities(PSYSTEM_POWER_CAPABILITIES lpspc) {
	return OriginalGetPwrCapabilities(lpspc); 
}

extern "C"
__declspec(dllexport) NTSTATUS __stdcall CallNtPowerInformation(POWER_INFORMATION_LEVEL InformationLevel, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength){
	return OriginalCallNtPowerInformation(InformationLevel, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
}

extern "C"
__declspec(dllexport) POWER_PLATFORM_ROLE PowerDeterminePlatformRole(){
	return OriginalPowerDeterminePlatformRole();
}


void Fail(const char *){


};

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
			Fail("failed to load POWRPROF.dll");
		}
	}

	OriginalGetPwrCapabilities = (BOOLEAN(*)(PSYSTEM_POWER_CAPABILITIES))GetProcAddress(dllHandle, "GetPwrCapabilities");
	OriginalCallNtPowerInformation = (NTSTATUS (*)(POWER_INFORMATION_LEVEL, PVOID, ULONG, PVOID, ULONG))GetProcAddress(dllHandle, "CallNtPowerInformation");
	OriginalPowerDeterminePlatformRole = (POWER_PLATFORM_ROLE (*)())GetProcAddress(dllHandle, "PowerDeterminePlatformRole");
	if (OriginalGetPwrCapabilities == nullptr || OriginalCallNtPowerInformation == nullptr || OriginalPowerDeterminePlatformRole == nullptr) {
		Fail("Could not locate symbols in powrprof.dll");
	}

}



void CreateConsole()
{
	AllocConsole();

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



DWORD WINAPI DllThread(LPVOID lpParam){
	loadWrapper();



	while (GetModuleHandle("Sky.exe") == 0)
	{
		Sleep(100);
	}


	HMODULE mod = LoadLibrary(TEXT("Sky"));


	return EXIT_SUCCESS;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){


	switch (fdwReason) {
        DisableThreadLibraryCalls(hinstDLL);
        case DLL_PROCESS_ATTACH:
        	CreateConsole();
        	static HANDLE dllHandle = CreateThread(NULL, 0, DllThread, NULL, 0, NULL);
            break;
        case DLL_PROCESS_DETACH:
        
            break;
    }
    
	return TRUE;

}
