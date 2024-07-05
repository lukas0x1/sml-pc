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
	float fontSize;
    unsigned int unicodeRangeStart;
    unsigned int unicodeRangeEnd;
}fontconfig;

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
        static_cast<ImWchar>(fontconfig.unicodeRangeStart), static_cast<ImWchar>(fontconfig.unicodeRangeEnd),   // Dynamic Unicode range
        0  // end of list
        };
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

    void SMLMainMenu(){
        char buf[64];
        ig::SetNextWindowSize({200, 0}, ImGuiCond_Once);
        if(ig::Begin("SML Main")) {
            ig::BeginTable("##mods", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody);
            ig::TableSetupColumn("Mod", ImGuiTableColumnFlags_WidthStretch);
            ig::TableSetupColumn( "Info", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("Info").x);

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
            ImGuiIO& io = ImGui::GetIO();
            ImGui::Text("font's path %s", fontconfig.fontPath.c_str());
            ImGui::SameLine();
            ImGui::Text("font's Start Range is %u", fontconfig.unicodeRangeStart);
            ImGui::SameLine();
            ImGui::Text("font's End Ranse is %u", fontconfig.unicodeRangeEnd);
            ImGui::Text("Fonts: %d fonts, TexSize: width %d, height %d", io.Fonts->Fonts.Size, io.Fonts->TexWidth, io.Fonts->TexHeight);
            ShowFontSelector();
            
            const float MIN_SCALE = 0.3f;
            const float MAX_SCALE = 3.0f;
            static float window_scale = 1.0f;
            if (ImGui::DragFloat("window scale", &window_scale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp)) // Scale only this window
                ImGui::SetWindowFontScale(window_scale);
            ImGui::DragFloat("global scale", &io.FontGlobalScale, 0.005f, MIN_SCALE, MAX_SCALE, "%.2f", ImGuiSliderFlags_AlwaysClamp); // Scale everything
            ImGui::Text(
                    "Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / io.Framerate,
                    io.Framerate
            );
        }
        ig::End();
    }

    void Render( ) {
        if (!bShowMenu)
            return;
        SMLMainMenu();
        ModLoader::RenderAll();
        //ig::ShowDemoWindow( );
    }
} // namespace Menu
