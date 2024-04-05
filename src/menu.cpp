
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
        ig::Begin("SML Main");

            ig::BeginTabBar("##mods");

            for(int i = 0; i < ModLoader::GetModCount(); i++) {
                auto& info = ModLoader::GetModInfo(i);
                snprintf(buf, 64, "%s##%d", info.name.c_str(), i);
                if(ig::BeginTabItem(info.name.c_str( ))) {
                    snprintf(buf, 64, "enable##%d", i);
                    if(ig::Checkbox(buf, &ModLoader::GetModEnabled(i))) {
                        if(ModLoader::GetModEnabled(i)) {
                            ModLoader::EnableMod(i);
                        } else {
                            ModLoader::DisableMod(i);
                        }
                    }
                    ig::Text("%s", ModLoader::toString(i).c_str());
                    ModLoader::Render(i);
                }
            }
            ig::EndTabBar( );

        ig::End();
    }

    void Render( ) {
        if (!bShowMenu)
            return;
        SMLMainMenu();
        //ig::ShowDemoWindow( );
    }
} // namespace Menu
