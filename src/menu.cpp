
#include <imgui.h>
#include <imgui_impl_win32.h>
#include "include/menu.hpp"
#include "include/global_variables.h"
namespace ig = ImGui;
namespace gv = GlobalVariables;

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
        ig::Begin("SML Main");
            if(ig::Button("Login With Local Account", {-1, 0})) {
                uintptr_t accountManager = *(uintptr_t*)(gv::gamePtr + 0x1D8);
                uint32_t* accountType = (uint32_t*)(accountManager + 0xf64);
                *accountType = 0;
            }
        ig::End();
    }

    void Render( ) {
        if (!bShowMenu)
            return;
        SMLMainMenu();
        //ig::ShowDemoWindow( );
    }
} // namespace Menu
