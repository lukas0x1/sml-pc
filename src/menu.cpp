
#include <cstdio>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "include/menu.hpp"
#include "include/mod_loader.h"

namespace ig = ImGui;

namespace Menu {
    void InitializeContext(HWND hwnd) {
        if (ig::GetCurrentContext( ))
            return;

        ImGui::CreateContext( );
        ImGui_ImplWin32_Init(hwnd);

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
