#pragma once

#include <Windows.h>
#include <iostream>

namespace Menu {
    void InitializeContext(HWND hwnd);
    void Render( );

    inline bool bShowMenu = true;
    void setDllPath(const std::string& path);
} // namespace Menu
