#include <cstdio>
#include <iostream>
#include <fstream>
#include <format>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "include/menu.hpp"
#include "include/mod_loader.h"
#include "include/json.hpp"

using json = nlohmann::json;

namespace ig = ImGui;

struct FontConfig {
    std::string fontPath;
    float fontSize = 0.0f;
    unsigned int unicodeRangeStart = 0;
    unsigned int unicodeRangeEnd = 0;
} fontconfig;

namespace Menu {
    void loadFontConfig(const std::string& filename, FontConfig& fontconfig) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filename << std::endl;
            return;
        }

        json jsonData;
        try {
            file >> jsonData;

            // Read fontPath
            fontconfig.fontPath = jsonData["fontPath"].get<std::string>();

			// Read fontSize
			fontconfig.fontSize = jsonData["fontSize"].get<float>();

            // Read unicodeRangeStart and unicodeRangeEnd
            fontconfig.unicodeRangeStart = static_cast<ImWchar>(std::stoi(jsonData["unicodeRangeStart"].get<std::string>(), nullptr, 16));
            fontconfig.unicodeRangeEnd = static_cast<ImWchar>(std::stoi(jsonData["unicodeRangeEnd"].get<std::string>(), nullptr, 16));

        } catch (const json::exception& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }
        file.close();
    }

    void LoadFontsFromFolder(FontConfig& fontconfig) {
        static const ImWchar ranges[] = {
        static_cast<ImWchar>(fontconfig.unicodeRangeStart), static_cast<ImWchar>(fontconfig.unicodeRangeEnd), 0};
        ImGuiIO& io = ImGui::GetIO();

        namespace fs = std::filesystem;

        try {
            for (const auto& entry : fs::directory_iterator(fontconfig.fontPath)) {
                if (!entry.is_regular_file())
                    continue;

                std::string filename = entry.path().string();
                if (fs::path(filename).extension() == ".ttf" || fs::path(filename).extension() == ".otf") {
                    ImFontConfig Imfontconfig;
                    Imfontconfig.OversampleH = 3;
                    Imfontconfig.OversampleV = 3;
                    Imfontconfig.PixelSnapH = true;

                    // Attempt to load font
                    if (!io.Fonts->AddFontFromFileTTF(filename.c_str(), fontconfig.fontSize, &Imfontconfig, ranges)) {
                        std::cerr << "Failed to load font: " << filename << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Exception while loading fonts: " << e.what() << std::endl;
        }
    }

    void ShowFontSelector(){
        ImGuiIO& io = ImGui::GetIO();
        ImFont* font_current = ImGui::GetFont();
        if (ImGui::BeginCombo("Fonts", font_current->GetDebugName()))
        {
            for (ImFont* font : io.Fonts->Fonts)
            {
                ImGui::PushID((void*)font);
                if (ImGui::Selectable(font->GetDebugName(), font == font_current))
                    io.FontDefault = font;
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
    }

    void InitializeContext(HWND hwnd) {
        if (ig::GetCurrentContext( ))
            return;

        ImGui::CreateContext( );
        ImGui_ImplWin32_Init(hwnd);

        loadFontConfig("sml_config.json", fontconfig);
        LoadFontsFromFolder(fontconfig);

        ImGuiIO& io = ImGui::GetIO( );
        io.IniFilename = io.LogFilename = nullptr;
    }

    void HelpMarker(const char* description){ 
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(description);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void SMLMainMenu() {
        char buf[64];
        ig::SetNextWindowSize({200, 0}, ImGuiCond_Once);

        if(ig::Begin("SML Main",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::SeparatorText(("Mods (" + std::to_string(ModLoader::GetModCount()) + ")").c_str());
            ig::BeginTable("##mods", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody);
            ig::TableSetupColumn("Mod", ImGuiTableColumnFlags_WidthStretch);
            ig::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Info").x);

            for(int i = 0; i < ModLoader::GetModCount(); i++) {
                    snprintf(buf, 64, "%s##check%d", ModLoader::GetModName(i).data(), i);
                    ig::TableNextColumn();
                    if(ig::Checkbox(buf, &ModLoader::GetModEnabled(i))) {
                        if(ModLoader::GetModEnabled(i)) {
                            ModLoader::EnableMod(i);
                        } else {
                            ModLoader::DisableMod(i);
                        }
                    }
                    ig::TableNextColumn();
                    HelpMarker(ModLoader::toString(i).c_str());
            }

            ig::EndTable();
            ImGui::SeparatorText("Settings");
            ImGuiIO& io = ImGui::GetIO();
            ShowFontSelector();
            ImGui::SameLine();
            HelpMarker(std::format("Total: {}\nPath: {}\nStart Range: {}\nEnd Range: {}\nSize: {}W / {}H", io.Fonts->Fonts.Size, fontconfig.fontPath.c_str(), fontconfig.unicodeRangeStart, fontconfig.unicodeRangeEnd, io.Fonts->TexWidth, io.Fonts->TexHeight).c_str());
            
            const float MIN_SCALE = 0.3f;
            const float MAX_SCALE = 3.0f;
            static float window_scale = 1.0f;

            if (ImGui::DragFloat("Window scale", &window_scale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                ImGui::SetWindowFontScale(window_scale);

            ImGui::DragFloat("Global scale", &io.FontGlobalScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("FPS: %.1f / %.3f ms/frame", io.Framerate, 1000.0f / io.Framerate);
        }
        ig::End();
    }

    void Render( ) {
        if (!bShowMenu)
            return;
        SMLMainMenu();
        ModLoader::RenderAll();
    }
}