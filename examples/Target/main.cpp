#include <d3d9.h>
#include <string>
#include <tchar.h>
#include <iostream>
#include <assert.h>

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

//  Uncomment if you want a debug console in release mode
//#define REL_WITHDBG

//  Data
bool bFullScreen    = true;
int WndX            = 0;
int WndY            = 0;
int WndWidth        = 0;
int WndHeight       = 0;

static LPDIRECT3D9              g_pD3D          = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice    = nullptr;
static UINT                     g_ResizeWidth   = 0, 
                                g_ResizeHeight  = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Function to toggle fullscreen/windowed
void ToggleFullscreen(HWND hWnd, bool fullscreen);

//  Debug data and functions
#if defined(REL_WITHDBG) || defined(_DEBUG)

FILE* pStream = nullptr;

bool CreateConsole(std::wstring_view szConsoleTitle);
bool DestroyConsole();
#endif

//  Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Main code
int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
)
{
#if defined(REL_WITHDBG) || defined(_DEBUG)
    if (!CreateConsole(L"Debug Console"))
        return EXIT_FAILURE;
#endif

    WndWidth = GetSystemMetrics(SM_CXSCREEN);
    WndHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Target", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, L"Target Window", WS_POPUP, WndX, WndY, WndWidth, WndHeight, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return EXIT_FAILURE;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& Io     = ImGui::GetIO(); (void)Io;
    Io.IniFilename  = nullptr;
    Io.LogFilename  = nullptr;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth     = g_ResizeWidth;
            g_d3dpp.BackBufferHeight    = g_ResizeHeight;
            g_ResizeWidth               = g_ResizeHeight = 0;
            ResetDevice();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize({ 400, 200 });
        ImGui::Begin("Target Window");
        {
            static bool bPrevFullScreen = bFullScreen;
            ImGui::Checkbox("Fullscreen", &bFullScreen);

            if (bPrevFullScreen != bFullScreen)
            {
                ToggleFullscreen(hwnd, bFullScreen);
                bPrevFullScreen = bFullScreen;
            }

            if (ImGui::SliderInt("Wnd Width", &WndWidth, 800, GetSystemMetrics(SM_CXSCREEN)))
                ::SetWindowPos(hwnd, 0, 0, 0, WndWidth, WndHeight, SWP_FRAMECHANGED);

            if (ImGui::SliderInt("Wnd Height", &WndHeight, 600, GetSystemMetrics(SM_CYSCREEN)))
                ::SetWindowPos(hwnd, 0, 0, 0, WndWidth, WndHeight, SWP_FRAMECHANGED);

            ImGui::End();
        }

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(70, 30, 50, 255), 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

#if defined(REL_WITHDBG) || defined(_DEBUG)
    if (!DestroyConsole)
        return EXIT_FAILURE;
#endif

    return EXIT_SUCCESS;
}

// Function to toggle fullscreen/windowed
void ToggleFullscreen(HWND hWnd, bool fullscreen)
{
    if (fullscreen)
    {
        ::SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        ::SetWindowPos(hWnd, HWND_TOP, 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
    }
    else
    {
        ::SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        ::SetWindowPos(hWnd, HWND_TOP, 0, 0, WndWidth, WndHeight, SWP_FRAMECHANGED);
    }
}

//  Debug functions

#if defined(REL_WITHDBG) || defined(_DEBUG)
bool CreateConsole(std::wstring_view szConsoleTitle)
{
    if (!AllocConsole())
        return false;

    if (freopen_s(&pStream, "CONOUT$", "w", stdout) != 0)
        return false;

    //  set console size
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    const SMALL_RECT c = { 0, 0, 55, 20 };

    if (!SetConsoleWindowInfo(hConsole, TRUE, &c))
        return false;

    if (!SetConsoleTitleW(szConsoleTitle.data()))
        false;

    return true;
}

bool DestroyConsole()
{
    fclose(pStream);

    if (!FreeConsole())
        return false;

    HWND hWndConsole = GetConsoleWindow();

    if (hWndConsole != nullptr)
        PostMessageW(hWndConsole, WM_CLOSE, 0U, 0L);

    return true;
}
#endif

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) 
    { 
        g_pd3dDevice->Release(); 
        g_pd3dDevice = nullptr; 
    }

    if (g_pD3D) 
    { 
        g_pD3D->Release(); 
        g_pD3D = nullptr;
    }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;

        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
