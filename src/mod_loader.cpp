#include <filesystem>

#include "include/mod_loader.h"


std::vector<ModItem> ModLoader::mods;

void ModLoader::LoadMods() {
    
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) { // 判断是否是普通文件
            std::string filePath = entry.path().string();
            std::string fileExtension = entry.path().extension().string();

            if (fileExtension == ".dll") { // 判断是否是DLL文件
                HMODULE hModule = LoadLibraryA(filePath.c_str()); // 加载DLL文件
                if (hModule != nullptr) {
                    loadedModules.push_back(hModule); // 将加载的DLL模块句柄保存到向量中
                } else {
                    std::cerr << "Failed to load DLL: " << filePath << std::endl;
                }
            }
        }
    }

    // 在这里可以对加载的DLL进行其他操作，如查找并调用其中的函数等

    // 可选：释放所有已加载的DLL模块句柄
    for (HMODULE hModule : loadedModules) {
        FreeLibrary(hModule);
    }
}