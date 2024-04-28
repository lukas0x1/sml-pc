

#include <Windows.h>

#include <cstdio>
#include <d3d11.h>
#include <dxgi1_2.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include <memory>

#include "include/dx11_hooks.hpp"

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <libmem/libmem.h>



#include "include/menu.hpp"

static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static ID3D11RenderTargetView* g_pd3dRenderTarget = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;

static void CleanupDeviceD3D11( );
static void CleanupRenderTarget( );
static void RenderImGui_DX11(IDXGISwapChain* pSwapChain);

static bool CreateDeviceD3D11(HWND hWnd) {
    // Create the D3DDevice
    DXGI_SWAP_CHAIN_DESC swapChainDesc = { };
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;

    const D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_NULL, NULL, 0, featureLevels, 2, D3D11_SDK_VERSION, &swapChainDesc, &g_pSwapChain, &g_pd3dDevice, nullptr, nullptr);
    if (hr != S_OK) {
        printf("[!] D3D11CreateDeviceAndSwapChain() failed. [rv: %lu]\n", hr);
        return false;
    }

    return true;
}


int GetCorrectDXGIFormat(int eCurrentFormat) {
    switch (eCurrentFormat) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM;
    }

    return eCurrentFormat;
}

static void CreateRenderTarget(IDXGISwapChain* pSwapChain) {
    ID3D11Texture2D* pBackBuffer = NULL;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer) {
        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);

        D3D11_RENDER_TARGET_VIEW_DESC desc = { };
        desc.Format = static_cast<DXGI_FORMAT>(GetCorrectDXGIFormat(sd.BufferDesc.Format));
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &desc, &g_pd3dRenderTarget);
        pBackBuffer->Release( );
    }
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain*, UINT, UINT)> oPresent;
static HRESULT WINAPI hkPresent(IDXGISwapChain* pSwapChain,
                                UINT SyncInterval,
                                UINT Flags) {


    RenderImGui_DX11(pSwapChain);

    return oPresent(pSwapChain, SyncInterval, Flags);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*)> oPresent1;
static HRESULT WINAPI hkPresent1(IDXGISwapChain* pSwapChain,
                                 UINT SyncInterval,
                                 UINT PresentFlags,
                                 const DXGI_PRESENT_PARAMETERS* pPresentParameters) {
    RenderImGui_DX11(pSwapChain);

    return oPresent1(pSwapChain, SyncInterval, PresentFlags, pPresentParameters);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT)> oResizeBuffers;
static HRESULT WINAPI hkResizeBuffers(IDXGISwapChain* pSwapChain,
                                      UINT BufferCount,
                                      UINT Width,
                                      UINT Height,
                                      DXGI_FORMAT NewFormat,
                                      UINT SwapChainFlags) {
    CleanupRenderTarget( );

    return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT, const UINT*, IUnknown* const*)> oResizeBuffers1;
static HRESULT WINAPI hkResizeBuffers1(IDXGISwapChain* pSwapChain,
                                       UINT BufferCount,
                                       UINT Width,
                                       UINT Height,
                                       DXGI_FORMAT NewFormat,
                                       UINT SwapChainFlags,
                                       const UINT* pCreationNodeMask,
                                       IUnknown* const* ppPresentQueue) {
    CleanupRenderTarget( );

    return oResizeBuffers1(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags, pCreationNodeMask, ppPresentQueue);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**)> oCreateSwapChain;
static HRESULT WINAPI hkCreateSwapChain(IDXGIFactory* pFactory,
                                        IUnknown* pDevice,
                                        DXGI_SWAP_CHAIN_DESC* pDesc,
                                        IDXGISwapChain** ppSwapChain) {
    CleanupRenderTarget( );

    return oCreateSwapChain(pFactory, pDevice, pDesc, ppSwapChain);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForHwnd;
static HRESULT WINAPI hkCreateSwapChainForHwnd(IDXGIFactory* pFactory,
                                               IUnknown* pDevice,
                                               HWND hWnd,
                                               const DXGI_SWAP_CHAIN_DESC1* pDesc,
                                               const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
                                               IDXGIOutput* pRestrictToOutput,
                                               IDXGISwapChain1** ppSwapChain) {
    CleanupRenderTarget( );

    return oCreateSwapChainForHwnd(pFactory, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForCoreWindow;
static HRESULT WINAPI hkCreateSwapChainForCoreWindow(IDXGIFactory* pFactory,
                                                     IUnknown* pDevice,
                                                     IUnknown* pWindow,
                                                     const DXGI_SWAP_CHAIN_DESC1* pDesc,
                                                     IDXGIOutput* pRestrictToOutput,
                                                     IDXGISwapChain1** ppSwapChain) {
    CleanupRenderTarget( );

    return oCreateSwapChainForCoreWindow(pFactory, pDevice, pWindow, pDesc, pRestrictToOutput, ppSwapChain);
}

static std::add_pointer_t<HRESULT WINAPI(IDXGIFactory*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*, IDXGIOutput*, IDXGISwapChain1**)> oCreateSwapChainForComposition;
static HRESULT WINAPI hkCreateSwapChainForComposition(IDXGIFactory* pFactory,
                                                      IUnknown* pDevice,
                                                      const DXGI_SWAP_CHAIN_DESC1* pDesc,
                                                      IDXGIOutput* pRestrictToOutput,
                                                      IDXGISwapChain1** ppSwapChain) {
    CleanupRenderTarget( );

    return oCreateSwapChainForComposition(pFactory, pDevice, pDesc, pRestrictToOutput, ppSwapChain);
}

namespace DX11 {
    void Hook(HWND hwnd) {

        

        if (!CreateDeviceD3D11(hwnd)) {
            printf("[!] CreateDeviceD3D11() failed.\n");
            return;
        }

        printf("[+] DirectX11: g_pd3dDevice: 0x%p\n", g_pd3dDevice);
        printf("[+] DirectX11: g_pSwapChain: 0x%p\n", g_pSwapChain);

        if (g_pd3dDevice) {
            Menu::InitializeContext(hwnd);

            // Hook
            IDXGIDevice* pDXGIDevice = NULL;
            g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));

            IDXGIAdapter* pDXGIAdapter = NULL;
            pDXGIDevice->GetAdapter(&pDXGIAdapter);

            IDXGIFactory* pIDXGIFactory = NULL;
            pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory));

            if (!pIDXGIFactory) {
                printf("[!] pIDXGIFactory is NULL.\n");
                return;
            }

            pIDXGIFactory->Release( );
            pDXGIAdapter->Release( );
            pDXGIDevice->Release( );

            void** pVTable = *reinterpret_cast<void***>(g_pSwapChain);
            void** pFactoryVTable = *reinterpret_cast<void***>(pIDXGIFactory);

            void* fnCreateSwapChain = pFactoryVTable[10];
            void* fnCreateSwapChainForHwndChain = pFactoryVTable[15];
            void* fnCreateSwapChainForCWindowChain = pFactoryVTable[16];
            void* fnCreateSwapChainForCompChain = pFactoryVTable[24];

            void* fnPresent = pVTable[8];
            void* fnPresent1 = pVTable[22];

            void* fnResizeBuffers = pVTable[13];
            void* fnResizeBuffers1 = pVTable[39];

            CleanupDeviceD3D11( );


            lm_module_t mod;
            if(LM_FindModule("gameoverlayrenderer64.dll", &mod)){
                lm_address_t present_hk_sig = LM_SigScan("48 89 6C 24 18 48 89 74 24 20 41 56 48 83 EC ? 41 8B E8", mod.base, mod.size);      
                if(!present_hk_sig){
                    printf("failed to find present_hk");
                    return;
                }
                static lm_size_t presentStatus = LM_HookCode(present_hk_sig, reinterpret_cast<lm_address_t>(&hkPresent), reinterpret_cast<lm_address_t *>(&oPresent));
            }

            else{
                static lm_size_t presentStatus = LM_HookCode(reinterpret_cast<lm_address_t>(fnPresent), reinterpret_cast<lm_address_t>(&hkPresent), reinterpret_cast<lm_address_t *>(&oPresent));
                static lm_size_t present1Status = LM_HookCode(reinterpret_cast<lm_address_t>(fnPresent1), reinterpret_cast<lm_address_t>(&hkPresent1), reinterpret_cast<lm_address_t *>(&oPresent1));
            }

            static lm_size_t cscStatus = LM_HookCode(reinterpret_cast<lm_address_t>(fnCreateSwapChain), reinterpret_cast<lm_address_t>(&hkCreateSwapChain), reinterpret_cast<lm_address_t *>(&oCreateSwapChain));
            static lm_size_t cschStatus = LM_HookCode(reinterpret_cast<lm_address_t>(fnCreateSwapChainForHwndChain), reinterpret_cast<lm_address_t>(&hkCreateSwapChainForHwnd), reinterpret_cast<lm_address_t *>(&oCreateSwapChainForHwnd));
            static lm_size_t csccwStatus = LM_HookCode(reinterpret_cast<lm_address_t>(fnCreateSwapChainForCWindowChain), reinterpret_cast<lm_address_t>(&hkCreateSwapChainForCoreWindow), reinterpret_cast<lm_address_t *>(&oCreateSwapChainForCoreWindow));
            static lm_size_t csccStatus = LM_HookCode(reinterpret_cast<lm_address_t>(fnCreateSwapChainForCompChain), reinterpret_cast<lm_address_t>(&hkCreateSwapChainForComposition), reinterpret_cast<lm_address_t *>(&oCreateSwapChainForComposition));

            

            static lm_size_t resizeStatus = LM_HookCode(reinterpret_cast<lm_address_t>(fnResizeBuffers), reinterpret_cast<lm_address_t>(&hkResizeBuffers), reinterpret_cast<lm_address_t *>(&oResizeBuffers));
            static lm_size_t resize1Status = LM_HookCode(reinterpret_cast<lm_address_t>(fnResizeBuffers1), reinterpret_cast<lm_address_t>(&hkResizeBuffers1), reinterpret_cast<lm_address_t *>(&oResizeBuffers1));
        }
    }

    void Unhook( ) {
        if (ImGui::GetCurrentContext( )) {
            if (ImGui::GetIO( ).BackendRendererUserData)
                ImGui_ImplDX11_Shutdown( );

            if (ImGui::GetIO( ).BackendPlatformUserData)
                ImGui_ImplWin32_Shutdown( );

            ImGui::DestroyContext( );
        }

        CleanupDeviceD3D11( );
    }
} // namespace DX11

static void CleanupRenderTarget( ) {
    if (g_pd3dRenderTarget) {
        g_pd3dRenderTarget->Release( );
        g_pd3dRenderTarget = NULL;
    }
}

static void CleanupDeviceD3D11( ) {
    CleanupRenderTarget( );

    if (g_pSwapChain) {
        g_pSwapChain->Release( );
        g_pSwapChain = NULL;
    }
    if (g_pd3dDevice) {
        g_pd3dDevice->Release( );
        g_pd3dDevice = NULL;
    }
    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release( );
        g_pd3dDeviceContext = NULL;
    }
}

static void RenderImGui_DX11(IDXGISwapChain* pSwapChain) {
    if (!ImGui::GetIO( ).BackendRendererUserData) {
        if (SUCCEEDED(pSwapChain->GetDevice(IID_PPV_ARGS(&g_pd3dDevice)))) {
            g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
            ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        }
    }

    if (!false) {
        if (!g_pd3dRenderTarget) {
            CreateRenderTarget(pSwapChain);
        }

        if (ImGui::GetCurrentContext( ) && g_pd3dRenderTarget) {
            ImGui_ImplDX11_NewFrame( );
            ImGui_ImplWin32_NewFrame( );
            ImGui::NewFrame( );

            Menu::Render( );

            ImGui::Render( );

            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pd3dRenderTarget, NULL);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData( ));
        }
    }
}
