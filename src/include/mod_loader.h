#include <windows.h>
#include <vector>

#include "api.h"


struct ModItem {
    HMODULE hModule;
    ModInfo info;
};

class ModLoader {
    static std::vector<ModItem> mods;

public:
    static void LoadMods();
};