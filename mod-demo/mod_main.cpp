
#include <windows.h>

#include "include/imgui.h"
#include "include/api.h"
#include "include/mod_main.h"
#include "include/global_variables.h"
#include "include/cpr/cpr.h"

namespace gv = GlobalVariables;


void GetModInfo(ModInfo& info) {
    info.author = MOD_AUTHOR;
    info.name = MOD_NAME;
    info.description = MOD_DESCRIPTION;
    info.version = MOD_VERSION;
} 


void Start() {
    gv::InitGlobalVariables(API.GetSkyBase());


    cpr::Response r = cpr::Get(cpr::Url{"https://api.github.com/repos/whoshuu/cpr/contributors"},
    cpr::Authentication{"user", "pass", cpr::AuthMode::BASIC},
    cpr::Parameters{{"anon", "true"}, {"key", "value"}});
    r.status_code;                  // 200
    r.header["content-type"];       // application/json; charset=utf-8
    r.text;                
}
    
void onEnable(){

}

void onDisable(){
    
}

void Render() {
    if(ImGui::Begin(MOD_NAME)) {
        if(ImGui::Button("Login With Local Account", {-1, 0})) {
            uintptr_t accountManager = *(uintptr_t*)(gv::gamePtr + 0x1D8);
            uint32_t* accountType = (uint32_t*)(accountManager + 0xf64);
            *accountType = 0;
        }
        ImGui::End();
    }

}