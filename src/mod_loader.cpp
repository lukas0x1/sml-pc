#include <windows.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <imgui.h>
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

    DWORD ftyp = GetFileAttributesA(directory.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES) {
        printf("Creating mods directory...\n");
        CreateDirectoryA((LPCSTR)directory.c_str(), NULL);
    }

    std::cout << "Loading mods from directory: " << directory << std::endl;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) { // is regular file
            std::string filePath = entry.path().string();
            std::string fileExtension = entry.path().extension().string();

            if (fileExtension == ".dll") { // is dll file
                std::cout << "Attempting to load DLL: " << filePath << std::endl;

                HMODULE hModule = LoadLibraryEx(filePath.c_str(), NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR); // load dll
                if (hModule != nullptr) {
                    mods.emplace_back(hModule);
                    ModItem& item = mods.back();

                    item.start = (StartFn)GetProcAddress(hModule, "Start");
                    if (item.start == nullptr) {
                        DWORD error = GetLastError();
                        std::cout << "Failed to load 'Start' function. Error code: " << error << std::endl;
                    }

                    item.onDisable = (OnDisableFn)GetProcAddress(hModule, "onDisable");
                    if (item.onDisable == nullptr) {
                        DWORD error = GetLastError();
                        std::cout << "Failed to load 'onDisable' function. Error code: " << error << std::endl;
                    }

                    item.onEnable = (OnEnableFn)GetProcAddress(hModule, "onEnable");
                    if (item.onEnable == nullptr) {
                        DWORD error = GetLastError();
                        std::cout << "Failed to load 'onEnable' function. Error code: " << error << std::endl;
                    }

                    item.getInfo = (GetModInfoFn)GetProcAddress(hModule, "GetModInfo");
                    if (item.getInfo == nullptr) {
                        DWORD error = GetLastError();
                        std::cout << "Failed to load 'GetModInfo' function. Error code: " << error << std::endl;
                    }

                    item.render = (RenderFn)GetProcAddress(hModule, "Render");
                    if (item.render == nullptr) {
                        DWORD error = GetLastError();
                        std::cout << "Failed to load 'Render' function. Error code: " << error << std::endl;
                    }

                    if (item.getInfo != nullptr) {
                        item.getInfo(item.info);
                    }
                    if (item.start != nullptr) {
                        item.start();
                    }
                }
                else {
                    DWORD error = GetLastError();
                    char* messageBuffer = nullptr;
                    size_t size = FormatMessageA(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                        NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
                    std::string message(messageBuffer, size);
                    LocalFree(messageBuffer);
                    std::cout << "Failed to load DLL: " << filePath << ". Error code: " << error << ". Message: " << message << std::endl;
					std::cout << "Press any key to continue..." << std::endl;
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
    if (mods[index].enabled && mods[index].render != nullptr)
        mods[index].render();
}

void ModLoader::EnableMod(int index) {
    if (mods[index].onEnable != nullptr)
        mods[index].onEnable();
}

void ModLoader::DisableMod(int index) {
    if (mods[index].onDisable != nullptr)
        mods[index].onDisable();
}

bool& ModLoader::GetModEnabled(int index) {
    return mods[index].enabled;
}

std::string_view ModLoader::GetModName(int index) {
    return mods[index].info.name;
}

void ModLoader::RenderAll() {
    for (int i = 0; i < mods.size(); i++) {
        Render(i);
    }
}

std::string ModLoader::toString(int index) {
    std::stringstream ss;

    ss << "Mod Information" << "\n";
    ss << "Name: " << mods[index].info.name << "\n";
    ss << "Version: " << mods[index].info.version << "\n";
    ss << "Author: " << mods[index].info.author << "\n";
    ss << "Description: " << mods[index].info.description << "\n";
    return ss.str();
}
