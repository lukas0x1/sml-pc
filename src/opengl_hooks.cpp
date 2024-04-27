#include <Windows.h>

#include <cstdio>
#include <memory>

#include "include/opengl_hooks.hpp"

#include <imgui_impl_opengl3.h>
#include <imgui_impl_win32.h>
#include <libmem/libmem.h>



#include "include/menu.hpp"

static std::add_pointer_t<BOOL WINAPI(HDC)> oWglSwapBuffers;
static BOOL WINAPI hkWglSwapBuffers(HDC Hdc) {
    if (ImGui::GetCurrentContext( )) {
        if (!ImGui::GetIO( ).BackendRendererUserData)
            ImGui_ImplOpenGL3_Init( );

        ImGui_ImplOpenGL3_NewFrame( );
        ImGui_ImplWin32_NewFrame( );
        ImGui::NewFrame( );

        Menu::Render( );

        ImGui::Render( );
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData( ));
    }

    return oWglSwapBuffers(Hdc);
}

namespace GL {
    void Hook(HWND hwnd) {
        HMODULE openGL32 = GetModuleHandleA("opengl32.dll");
        if (openGL32) {
            printf("[+] OpenGL32: ImageBase: 0x%p\n", openGL32);

            void* fnWglSwapBuffers = reinterpret_cast<void*>(GetProcAddress(openGL32, "wglSwapBuffers"));
            if (fnWglSwapBuffers) {
                Menu::InitializeContext(hwnd);

                // Hook
                printf("[+] OpenGL32: fnWglSwapBuffers: 0x%p\n", fnWglSwapBuffers);

                static lm_size_t status = LM_HookCode(reinterpret_cast<lm_address_t>(fnWglSwapBuffers), reinterpret_cast<lm_address_t>(&hkWglSwapBuffers), reinterpret_cast<lm_address_t *>(&oWglSwapBuffers));

                
            }
        }
    }

    void Unhook( ) {
        if (ImGui::GetCurrentContext( )) {
            if (ImGui::GetIO( ).BackendRendererUserData)
                ImGui_ImplOpenGL3_Shutdown( );

            if (ImGui::GetIO( ).BackendPlatformUserData)
                ImGui_ImplWin32_Shutdown( );

            ImGui::DestroyContext( );
        }
    }
} // namespace GL
