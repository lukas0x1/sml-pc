#include <cstddef>
#include <cstdint>
#include <Windows.h>
#include <psapi.h>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <libmem.h>
#include "include/api.h"

struct HookDef {
    uintptr_t addr;
    void* newFn;
    void** oldFn;
    uint64_t size;
};

std::unordered_map<uintptr_t, HookDef> hooks;
std::unordered_map<uintptr_t, std::vector<unsigned char>> OriginalBytes;
std::unordered_map<uintptr_t, std::vector<unsigned char>> patches;

ModApi* ModApi::instance = NULL;


ModApi& ModApi::Instance() {
    if (instance == NULL) instance = new ModApi;
    return *instance;
}

ModApi::ModApi() {
    skyBase = 0;
}

void ModApi::InitSkyBase() {
    /*
        while (GetModuleHandle("Sky.exe") == 0)
        Sleep(100);
        skyBase = (uintptr_t)LoadLibrary(TEXT("Sky.exe"));
    */
    // lm_module_t mod;
    // while(LM_LoadModule("Sky.exe", &mod) == 0) Sleep(100);
    // skyBase = mod.base;
    // skySize = mod.size;
    uintptr_t Sky_Base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

    size_t Sky_Size = []() -> size_t {
        MODULEINFO moduleInfo;
        if (GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &moduleInfo, sizeof(MODULEINFO))) {
            return moduleInfo.SizeOfImage;
        }
        return 0;
        }();
}

uintptr_t ModApi::GetSkyBase() {
    return skyBase;
}

uintptr_t ModApi::GetSkySize() {
    return skySize;
}

uintptr_t ModApi::Scan(const char* signature) {
    return LM_SigScan(signature, skyBase, skySize);
}

uintptr_t ModApi::Scan(const char* signature, uintptr_t start, size_t size) {
    return LM_SigScan(signature, start, size);
}

uintptr_t ModApi::ScanPattern(lm_bytearr_t pattern, const char* masking) {
    return LM_PatternScan(pattern, masking, skyBase, skySize);
}

uintptr_t ModApi::ScanPattern(lm_bytearr_t pattern, const char* masking, uintptr_t start, size_t size) {
    return LM_PatternScan(pattern, masking, start, size);
}

uintptr_t ModApi::ScanData(lm_bytearr_t data, size_t scansize) {
    return LM_DataScan(data, skySize, skyBase, scansize);
}

uintptr_t ModApi::ScanData(lm_bytearr_t data, size_t size, uintptr_t start, size_t scansize) {
    return LM_DataScan(data, size, start, scansize);
}

bool ModApi::Hook(uintptr_t addr, void* newFn, void** oldFn) {
    //todo: check if already hooked，Multiple modification hooks for the same function (wow i must suck at this!Q)
    HookDef hook;

    if (hooks.contains(addr)) { // dogshit_check.cpp
        printf("hooking : function is already hooked!");
        return true;
    }
    hook.addr = addr; hook.newFn = newFn; hook.oldFn = oldFn;

    hook.size = LM_HookCode(addr, (lm_address_t)newFn, (lm_address_t*)oldFn);
    if (!hook.size)
        return false;

    hooks[addr] = hook;
    return true;
}

bool ModApi::UnHook(uintptr_t addr) {
    auto it = hooks.begin();
    if ((it = hooks.find(addr)) != hooks.end()) {
        if (LM_UnhookCode(addr, (lm_address_t) * (it->second.oldFn), it->second.size)) {
            hooks.erase(addr);
            return true;
        }
    }
    return false;
}


bool ModApi::Patch(uintptr_t address, const std::vector<unsigned char>& patchBytes, bool toggle) {
    address += skyBase; // Ensure address is relative to Sky_Base

    // Check if the patch is stored in the Original Bytes map
    auto it = OriginalBytes.find(address);
    if (it == OriginalBytes.end()) {
        size_t size = patchBytes.size();
        // Change the protection to ReadWriteable
        if (!LM_ProtMemory(address, size, LM_PROT_XRW, NULL)) {
            std::cerr << "Failed to Change protection at address: " << std::hex << address << std::endl;
            return false;
        }

        // Store the original bytes
        std::vector<unsigned char> originalBytes(size);
        if (!LM_ReadMemory(address, originalBytes.data(), size)) {
            std::cerr << "Failed to read memory at address: " << std::hex << address << std::endl;
            return false;
        }

        // Save original bytes in the map
        OriginalBytes[address] = originalBytes;
    }

    const unsigned char* dataPtr = toggle ? patchBytes.data() : OriginalBytes[address].data();
    if (!LM_WriteMemory(address, dataPtr, OriginalBytes[address].size())) {
        std::cerr << "Failed to write memory at address: " << std::hex << address << std::endl;
        return false;
    }

    return true;
}

bool ModApi::Unpatch(uintptr_t address, const std::vector<unsigned char>& unpatchBytes) {
    size_t size = unpatchBytes.size();
    if (!LM_FreeMemory(address, size)) {
        std::cerr << "Failed to free memory at address: " << std::hex << address << std::endl;
        return true;
    }

    return false;
}

// fastfunctions (basically functions checkless counterparts)
// WARNING : fastfunctions should only be used for patches if you are extremely confident that it wont kill your game.
bool ModApi::FastHook(uintptr_t addr, void* newFn, void** oldFn) {
    HookDef hook;

    hook.addr = addr; hook.newFn = newFn; hook.oldFn = oldFn;
    hook.size = LM_HookCode(addr, (lm_address_t)newFn, (lm_address_t*)oldFn);

    hooks[addr] = hook;
}

bool ModApi::FastPatch(uintptr_t address, const std::vector<unsigned char>& patchBytes, bool toggle) {
    address += skyBase;

    auto it = OriginalBytes.find(address);
    if (it == OriginalBytes.end()) {
        size_t size = patchBytes.size();
        LM_ProtMemory(address, size, LM_PROT_XRW, NULL);

        std::vector<unsigned char> originalBytes(size);
        LM_ReadMemory(address, originalBytes.data(), size);
    }

    const unsigned char* dataPtr = toggle ? patchBytes.data() : OriginalBytes[address].data();
    LM_WriteMemory(address, dataPtr, OriginalBytes[address].size());
    return true;
}


bool ModApi::FastUnpatch(uintptr_t address, const std::vector<unsigned char>& unpatchBytes) {
    size_t size = unpatchBytes.size();
    LM_FreeMemory(address, size);

    return false;
}
