#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#ifdef _MSC_VER
    #define MOD_API __declspec(dllexport)
#else   
    #define MOD_API __attribute__((visibility("default")))
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

class MOD_API ModApi {
protected:
    static ModApi *instance;
    uintptr_t skyBase;
    size_t skySize;

public:
    static ModApi& Instance();

    ModApi();

    void InitSkyBase();

    uintptr_t GetSkyBase();
    uintptr_t GetSkySize();

    uintptr_t Scan(const char *signature);
    uintptr_t Scan(const char *signature, uintptr_t start, size_t size);

    bool Hook(uintptr_t addr, void* newFn, void** oldFn);

    bool UnHook(uintptr_t addr);

    bool Patch(uintptr_t address, const std::vector<unsigned char>& patchBytes, bool toggle = true);

};