#include <imgui.h>

#include <windows.h>
#include "include/api.h"

void EXPORT GetModInfo(ModInfo& info) {
    info.author = "yeahpm";
    info.name = "mod-demo";
    info.description = "A demo mod for the sml";
    info.version = "1.0.0";
} 


void EXPORT Render() {
    ImGui::Button("test");
}