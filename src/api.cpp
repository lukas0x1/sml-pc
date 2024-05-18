#include <cstddef>
#include <cstdint>
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
    if(instance == NULL) instance = new ModApi;
    return *instance;
}

ModApi::ModApi() {
    skyBase = 0;
}

void ModApi::InitSkyBase() {
/*
    while (GetModuleHandle("Sky.exe") == 0)Sleep(100);
	skyBase = (uintptr_t)LoadLibrary(TEXT("Sky.exe"));
*/
    lm_module_t mod;
    while(LM_LoadModule("Sky.exe", &mod) == 0) Sleep(100);
    skyBase = mod.base;
    skySize = mod.size;
}

uintptr_t ModApi::GetSkyBase() {
	return skyBase;
}

uintptr_t ModApi::GetSkySize() {
    return skySize;
}

uintptr_t ModApi::Scan(const char *signature){
    return LM_SigScan(signature, skyBase, skySize);
}

uintptr_t ModApi::Scan(const char *signature, uintptr_t start, size_t size){
    return LM_SigScan(signature, start, size);
}



bool ModApi::Hook(uintptr_t addr, void* newFn, void** oldFn) {
    //todo: check if already hooked，Multiple modification hooks for the same function
    HookDef hook;
    hook.addr = addr;
    hook.newFn = newFn;
    hook.oldFn = oldFn;
    hook.size = LM_HookCode(addr, (lm_address_t)newFn, (lm_address_t*)oldFn);
    if(!hook.size)
        return false;
    hooks[addr] = hook;
    return true;
}

bool ModApi::UnHook(uintptr_t addr) {
    //todo:
    auto it = hooks.begin();
	if ((it = hooks.find(addr)) != hooks.end()) {
        if(LM_UnhookCode(addr, (lm_address_t )*(it->second.oldFn), it->second.size)){
            hooks.erase(addr);
            return true;
        }
    }
    return false;
}


bool ModApi::DefPatch(uintptr_t address, const char* patchBytes){
    
    std::vector<unsigned char> PatchBytes;
    std::istringstream byteStream(patchBytes);

    unsigned int byteValue;
    while (byteStream >> std::hex >> byteValue) {
        PatchBytes.push_back(static_cast<unsigned char>(byteValue));
    }
    size_t size = PatchBytes.size();
    patches[address] = PatchBytes;

    // Change the protection to ReadWriteable
    if (!LM_ProtMemory(address, size,LM_PROT_XRW, NULL)) {
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

    return true;
}

bool ModApi::Patch(uintptr_t address) {

    // Check if the patch bytes are stored in the map
    auto it = patches.find(address);
    if (it == patches.end()) {
        std::cerr << "No Patch bytes found for address: " << std::hex << address << std::endl;
        return false;
    }

    std::vector<unsigned char>& PatchBytes = it->second;

    // Write the patch bytes
    if (!LM_WriteMemory(address, PatchBytes.data(), PatchBytes.size())) {
        std::cerr << "Failed to write memory at address: " << std::hex << address << std::endl;
        return false;
    }

    return true;
}

bool ModApi::Restore(uintptr_t address) {
    // Check if the original bytes are stored in the map
    auto it = OriginalBytes.find(address);
    if (it == OriginalBytes.end()) {
        std::cerr << "No original bytes found for address: " << std::hex << address << std::endl;
        return false;
    }

    const std::vector<unsigned char>& OriginalBytes = it->second;
    // Restore the original bytes
    if (!LM_WriteMemory(address, OriginalBytes.data(), OriginalBytes.size())) {
        std::cerr << "Failed to restore memory at address: " << std::hex << address << std::endl;
        return false;
    }

    return true;
}