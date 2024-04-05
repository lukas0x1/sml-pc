#include <filesystem>
#include <iostream>

#include "include/mod_loader.h"

typedef void (*startFn)();
typedef void (*getModinfoFn)(ModInfo& info);

std::vector<ModItem> ModLoader::mods;

void ModLoader::LoadMods() {
    
    std::string directory = "mods"; // 
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    if (pos != std::string::npos) {
        directory = std::string(buffer).substr(0, pos) + "/" + directory;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) { // is regular file
            std::string filePath = entry.path().string();
            std::string fileExtension = entry.path().extension().string();

            if (fileExtension == ".dll") { // is dll file
                HMODULE hModule = LoadLibraryA(filePath.c_str()); // load dll
                if (hModule != nullptr) {
                    ModItem item;
                    getModinfoFn getModinfo = (getModinfoFn)GetProcAddress(hModule, "GetModinfo");
                    getModinfo(item.info);
                    mods.push_back(std::move(item));

                } else {
                    std::cout << "Failed to load DLL: " << filePath << std::endl;
                }
            }
        }
    }
}