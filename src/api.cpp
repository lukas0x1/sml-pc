#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <libmem.h>
#include "include/api.h"

struct HookDef {
    uintptr_t addr;
    void* newFn;
    void** oldFn;
    uint64_t size;
};

std::unordered_map<uintptr_t, HookDef> hooks;

ModApi* ModApi::instance = NULL;


ModApi& ModApi::Instance() {
    if(instance == NULL) instance = new ModApi;
    return *instance;
}

ModApi::ModApi() {
    skyBase = 0;
}

void ModApi::InitSkyBase() {
	while (GetModuleHandle("Sky.exe") == 0)Sleep(100);
	skyBase = (uintptr_t)LoadLibrary(TEXT("Sky.exe"));
}

uintptr_t ModApi::GetSkyBase() {
	return skyBase;
}

void ModApi::Hook(uintptr_t addr, void* newFn, void** oldFn) {
    //todo: check if already hooked，Multiple modification hooks for the same function
    HookDef hook;
    hook.addr = addr;
    hook.newFn = newFn;
    hook.oldFn = oldFn;
    hook.size = LM_HookCode(addr, (lm_address_t)newFn, (lm_address_t*)oldFn);
    hooks[addr] = hook;
}

void ModApi::UnHook(uintptr_t addr) {
    //todo:
    auto it = hooks.begin();
	if ((it = hooks.find(addr)) != hooks.end()) {
        if(LM_UnhookCode(addr, (lm_address_t )*(it->second.oldFn), it->second.size))
            hooks.erase(addr);
    }
}
