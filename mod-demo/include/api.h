#include <cstdint>
#include <unordered_map>

#define HOOK_DEF(ret, name, ...) \
    ret (*orig_##name)(__VA_ARGS__); \
    ret my_##name(__VA_ARGS__)

#define HOOK_SET(addr, name) \
    ModApi::Instance().Hook(addr, (void*)my_##name, (void **)&orig_##name)


class ModApi {
public:
    static ModApi& Instance();

    ModApi();

    void InitSkyBase();

    uintptr_t GetSkyBase();

    void Hook(uintptr_t addr, void* newFn, void** oldFn);

    void UnHook(uintptr_t addr);
};

#define API = ModApi::Instance()