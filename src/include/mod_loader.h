#pragma once

#include <cstddef>
#include <windows.h>
#include <vector>

#include "api.h"

typedef void (*StartFn)();
typedef void (*GetModInfoFn)(ModInfo& info);
typedef void (*RenderFn)();
struct ModItem {
    HMODULE hModule;

    StartFn start;
    GetModInfoFn getInfo;
    RenderFn render;
    
    ModInfo info;


    ModItem(HMODULE hModule) : hModule(hModule), start(nullptr), getInfo(nullptr), render(nullptr) {}

};

class ModLoader {
    static std::vector<ModItem> mods;

public:
    static void LoadMods();
    static size_t GetModCount();
    static const ModInfo& GetModInfo(int index);
    static void CallModRender(int index);
    static std::string toString(int idx);
};