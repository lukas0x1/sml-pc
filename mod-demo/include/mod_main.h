#pragma once

#include "api.h"


#define MOD_NAME                "mod-demo" 
#define MOD_AUTHOR              "yeahpm"
#define MOD_DESCRIPTION         "A demo mod for the mod loader"
#define MOD_VERSION             "1.0.0"


extern "C" {
    void EXPORT GetModInfo(ModInfo& info);
    
    //This gets called right upon dll load. Setup code that needs to run early here. Don't define if you don't need to run code early
    void EXPORT Start();  
    
    //This gets called when the mod gets enabled in the menu. The setup code that doesn't need to run early should either go here or in imgui. 
    //If you don't define both this and the Render function, it won't give the option to enable the mod and only run Start. 
    void EXPORT onEnable(); 

    //Called when mod gets disabled
    void EXPORT onDisable();

    //Everything in here will get rendered in IMGUI. Don't declare if you don't want to render a menu.
    void EXPORT Render();
}