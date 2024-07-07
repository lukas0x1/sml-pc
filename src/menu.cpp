#include <cstdio>
#include <iostream>
#include <fstream>
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

            fontconfig.fontPath = jsonData["fontPath"].get<std::string>();
			fontconfig.fontSize = jsonData["fontSize"].get<float>();
            fontconfig.unicodeRangeStart = static_cast<ImWchar>(std::stoi(jsonData["unicodeRangeStart"].get<std::string>(), nullptr, 16));
            fontconfig.unicodeRangeEnd = static_cast<ImWchar>(std::stoi(jsonData["unicodeRangeEnd"].get<std::string>(), nullptr, 16));

        } catch (const json::exception& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }
        file.close();
    }

    void LoadFontsFromFolder(FontConfig& fontconfig) {
        static const ImWchar ranges[] = {
        static_cast<ImWchar>(fontconfig.unicodeRangeStart), static_cast<ImWchar>(fontconfig.unicodeRangeEnd), 0 };
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

    void SMLMainMenu() {
        char buf[64];
        ig::SetNextWindowSize({ 200, 0 }, ImGuiCond_Once);
        if (ig::Begin("SML Main")) {
            ImGui::SeparatorText("Mods");
            ig::BeginTable("##mods", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody);
            ig::TableSetupColumn("Mod", ImGuiTableColumnFlags_WidthStretch);
            ig::TableSetupColumn("Info", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Info").x);

            for (int i = 0; i < ModLoader::GetModCount(); i++) {
                snprintf(buf, 64, "%s##check%d", ModLoader::GetModName(i).data(), i);
                ig::TableNextColumn();
                if (ig::Checkbox(buf, &ModLoader::GetModEnabled(i))) {
                    if (ModLoader::GetModEnabled(i)) {
                        ModLoader::EnableMod(i);
                    }
                    else {
                        ModLoader::DisableMod(i);
                    }
                }
                ig::TableNextColumn();
                ig::TextDisabled("(?)");
                if (ig::BeginItemTooltip())
                {
                    ig::PushTextWrapPos(ig::GetFontSize() * 35.0f);
                    ig::TextUnformatted(ModLoader::toString(i).c_str());
                    ig::PopTextWrapPos();
                    ig::EndTooltip();
                }
            }
            ig::EndTable();
            ImGui::SeparatorText("Settings");
            ImGuiIO& io = ImGui::GetIO();
            ShowFontSelector();

            const float MIN_SCALE = 0.3f;
            const float MAX_SCALE = 3.0f;
            static float window_scale = 1.0f;

            if (ImGui::DragFloat("Window Scale", &window_scale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp))
                ImGui::SetWindowFontScale(window_scale);
            
            io.FontGlobalScale = 0.94f;
            ImGui::DragFloat("Global Scale", &io.FontGlobalScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Total Mods: %d", ModLoader::GetModCount());
            ImGui::SameLine();
            ImGui::Text("|");
            ImGui::SameLine();
            ImGui::Text("Total Fonts: %d", io.Fonts->Fonts.Size, io.Fonts->TexWidth, io.Fonts->TexHeight);
            ImGui::Text("%.1f FPS / %.3f ms/frame", io.Framerate, 1000.0f / io.Framerate);
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
