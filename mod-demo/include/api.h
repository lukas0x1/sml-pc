#pragma once

#include <cstdint>
#include <string>


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

    uintptr_t Scan(const char *signature);
    uintptr_t Scan(const char *signature, uintptr_t start, size_t size);
    
    void Hook(uintptr_t addr, void* newFn, void** oldFn);

    void UnHook(uintptr_t addr);
};

#define API ModApi::Instance()