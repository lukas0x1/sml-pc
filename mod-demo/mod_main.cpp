#include <imgui.h>
#include <windows.h>

#include "include/api.h"
#include "include/mod_main.h"
#include "include/global_variables.h"

namespace gv = GlobalVariables;


void GetModInfo(ModInfo& info) {
    info.author = MOD_AUTHOR;
    info.name = MOD_NAME;
    info.description = MOD_DESCRIPTION;
    info.version = MOD_VERSION;
} 


void Start() {
    gv::InitGlobalVariables(API.GetSkyBase());
}
    
void onEnable(){

}

void onDisable(){
    
}

void Render() {
    ImGui::Begin(MOD_NAME);
    if(ImGui::Button("Login With Local Account", {-1, 0})) {
        uintptr_t accountManager = *(uintptr_t*)(gv::gamePtr + 0x1D8);
        uint32_t* accountType = (uint32_t*)(accountManager + 0xf64);
        *accountType = 0;
    }
    ImGui::End();

}