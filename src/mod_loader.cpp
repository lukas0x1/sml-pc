#include <cstddef>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

#include "include/mod_loader.h"


std::vector<ModItem> ModLoader::mods;

void ModLoader::LoadMods() {
    
    std::string directory = "mods"; // 
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    if (pos != std::string::npos) {
        directory = std::string(buffer).substr(0, pos) + "\\" + directory;
    }

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) { // is regular file
            std::string filePath = entry.path().string();
            std::string fileExtension = entry.path().extension().string();

            if (fileExtension == ".dll") { // is dll file
                HMODULE hModule = LoadLibraryA(filePath.c_str()); // load dll
                if (hModule != nullptr) {
                    mods.emplace_back(hModule);
                    ModItem & item = mods.back();
                    item.start = (StartFn) GetProcAddress(hModule, "Start");
                    item.onDisable = (OnDisableFn) GetProcAddress(hModule, "onDisable");
                    item.onEnable = (OnEnableFn) GetProcAddress(hModule, "onEnable");
                    item.getInfo = (GetModInfoFn) GetProcAddress(hModule, "GetModInfo");
                    item.render = (RenderFn) GetProcAddress(hModule, "Render");
                    if(item.getInfo != NULL)
                        item.getInfo(item.info);
                    if(item.start != NULL) {
                        item.start();
                    }
                } else {
                    std::cout << "Failed to load DLL: " << filePath << std::endl;
                }
            }
        }
    }
}

size_t ModLoader::GetModCount() {
    return mods.size();
}


const ModInfo& ModLoader::GetModInfo(int index) {
    return mods[index].info;
}

void ModLoader::Render(int index) {
    if(mods[index].enabled && mods[index].render != nullptr)
        mods[index].render();
}
void ModLoader::EnableMod(int index) {
    if(mods[index].onEnable != nullptr)
        mods[index].onEnable();
}

void ModLoader::DisableMod(int index){
    if(mods[index].onDisable != nullptr)
        mods[index].onDisable();
}

bool& ModLoader::GetModEnabled(int index) {
    return mods[index].enabled;
}

std::string_view ModLoader::GetModName(int index) {
    return mods[index].info.name;
}


void ModLoader::RenderAll() {
    for(int i = 0 ; i < mods.size(); i++) {
        Render(i);
    }
}
std::string ModLoader::toString(int index) {
    std::stringstream ss;
    
    ss << "Mod Info: " << "\n";
    ss << "Name: " << mods[index].info.name << "\n";
    ss << "Version: " << mods[index].info.version << "\n";
    ss << "Author: " << mods[index].info.author << "\n";
    ss << "Description: " << mods[index].info.description << "\n";
    // ss << "GetModInfo: " << mods[index].getInfo << "\n";
    // ss << "Render: " << mods[index].render << "\n";
    return ss.str();
}

