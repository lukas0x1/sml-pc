#pragma once

#include <cstdint>
#include <string>

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

public:
    static ModApi& Instance();

    ModApi();

    void InitSkyBase();

    uintptr_t GetSkyBase();

    void Hook(uintptr_t addr, void* newFn, void** oldFn);

    void UnHook(uintptr_t addr);
};