#pragma once

#include <cstdint>
#include <string>
#include "libmem.h"

#ifdef _MSC_VER
    #define EXPORT __declspec(dllexport)
#else   
    #define EXPORT __attribute__((visibility("default")))
#endif

#define HOOK_DEF(ret, name, ...) \
    ret (*orig_##name)(__VA_ARGS__); \
    ret my_##name(__VA_ARGS__)

#define HOOK_SET(addr, name) \
    ModApi::Instance().Hook(addr, (void*)my_##name, (void **)&orig_##name)


typedef struct ModInfo {
    std::string name;
    std::string author;
    std::string description;
    std::string version;
}ModInfo;

class ModApi {
public:
    static ModApi& Instance();

    ModApi();

    void InitSkyBase();

    uintptr_t GetSkyBase();
    uintptr_t GetSkySize();

    uintptr_t Scan(const char* signature);
    uintptr_t Scan(const char* signature, uintptr_t start, size_t size);

    uintptr_t ScanPattern(lm_bytearr_t pattern, const char* masking);
    uintptr_t ScanPattern(lm_bytearr_t pattern, const char* masking, uintptr_t start, size_t size);

    uintptr_t ScanData(lm_bytearr_t data, size_t scansize);
    uintptr_t ScanData(lm_bytearr_t data, size_t size, uintptr_t start, size_t scansize);

    bool Hook(uintptr_t addr, void* newFn, void** oldFn);
    bool Patch(uintptr_t address, const std::vector<unsigned char>& patchBytes, bool toggle = true);
    bool Unpatch(uintptr_t address, const std::vector<unsigned char>& unpatchBytes);
    bool Restore(uintptr_t address);
    bool SetByte(uintptr_t address, std::vector<unsigned char> setByte);

    bool FastHook(uintptr_t addr, void* newFn, void** oldFn);
    bool FastPatch(uintptr_t address, const std::vector<unsigned char>& patchBytes, bool toggle = true);
    bool FastUnpatch(uintptr_t address, const std::vector<unsigned char>& unpatchBytes);
    bool FastRestore(uintptr_t address);
    bool FastSetByte(uintptr_t address, std::vector<unsigned char> setByte);

    bool UnHook(uintptr_t addr);
};

#define API ModApi::Instance()