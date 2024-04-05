#pragma once

#include <cstddef>
#include <windows.h>
#include <vector>

#include "api.h"

typedef void (*StartFn)();
typedef void (*OnEnableFn)();
typedef void (*OnDisableFn)();
typedef void (*StartFn)();
typedef void (*GetModInfoFn)(ModInfo& info);
typedef void (*RenderFn)();

enum Fn_Idx{
    START_FN = 0,
    GET_MOD_INFO_FN,
    RENDER_FN,
    ON_ENABLE_FN,
    ON_DISABLE_FN
};

struct ModItem {
    HMODULE hModule;
    StartFn start;
    GetModInfoFn getInfo;
    RenderFn render;
    OnEnableFn onEnable;
    OnDisableFn onDisable;

    ModInfo info;
    bool enabled;
    ModItem(HMODULE hModule) : hModule(hModule), start(nullptr), getInfo(nullptr), render(nullptr), onEnable(nullptr), onDisable(nullptr), enabled(false) {}
};

class ModLoader {
    static std::vector<ModItem> mods;

public:
    static void LoadMods();
    static size_t GetModCount();
    static const ModInfo& GetModInfo(int index);
    static bool& GetModEnabled(int index);

    static void Render(int index);
    static void EnableMod(int index);
    static void DisableMod(int index);

    static std::string toString(int idx);
};