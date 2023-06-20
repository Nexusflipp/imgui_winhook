#include <d3d9.h>
#include <string>
#include <thread>
#include <chrono>
#include <tchar.h>
#include <iostream>
#include <dwmapi.h>
#include <TlHelp32.h>

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_winhook.h"
#include "imgui_stdlib.h"

#pragma comment(lib, "dwmapi.lib")
using namespace std::chrono;

//  Uncomment if you want a debug console in release mode
//#define REL_WITHDBG

//  Target data
ULONG       TargetPid   = 0;
HWND        TargetWnd   = nullptr;
MARGINS	    Margin      = { -1 };
RECT        TargetRect  = { 0, 0, 0, 0 };

//  Data
static LPDIRECT3D9              g_pD3D          = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice    = nullptr;
static D3DPRESENT_PARAMETERS    g_d3dpp         = {};

//  Debug data and functions
#if defined(REL_WITHDBG) || defined(_DEBUG)

FILE* pStream = nullptr;

bool CreateConsole(std::wstring_view szConsoleTitle);
bool DestroyConsole();
#endif

//  Target finding functions
ULONG   GetProcId(std::wstring_view szProcName);
HWND    GetWnd(ULONG Pid);

//  Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();

//  Main code
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

    //  Find target
    while (TargetWnd == nullptr)
    {
        TargetPid = GetProcId(L"Target.exe");
        TargetWnd = GetWnd(TargetPid);
        std::this_thread::sleep_for(milliseconds(1));
    }

    //  Init target info
    ::GetClientRect(TargetWnd, &TargetRect);

    //  Create application window
    WNDCLASSEXW wc = { sizeof(wc), NULL, DefWindowProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, HBRUSH(RGB(0, 0, 0)), nullptr, L"Overlay", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowExW   (WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
                                    wc.lpszClassName, L"Overlay Window", WS_POPUP, TargetRect.left, TargetRect.top, TargetRect.right, TargetRect.bottom, nullptr, nullptr, wc.hInstance, nullptr);

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    DwmExtendFrameIntoClientArea(hwnd, &Margin);

    //  Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return EXIT_FAILURE;
    }

    //  Show the window
    ::ShowWindow(hwnd, SW_SHOW);
    ::UpdateWindow(hwnd);

    //  Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& Io     = ImGui::GetIO(); (void)Io;
    Io.IniFilename  = nullptr;
    Io.LogFilename  = nullptr;
    Io.ConfigFlags  |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    //  Setup Dear ImGui style
    ImGui::StyleColorsDark();

    //  Setup Platform/Renderer backends
    ImGui_ImplWinHook_Init(hwnd, TargetWnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    //  Should usually be done more often, this is just a poc. 
    //  More info on the methods can be found in the declaration of the func
    ImGui_ImplWinHook_DetectGamepad();

    //  Get Data
    ImGui_ImplWinHook_Data* Bd = ImGui_ImplWinHook_GetBackendData();

    //  Main loop
    bool Done = false;
    while (!Done)
    {
        //  Exit app if end was released
        if (ImGui::IsKeyReleased(ImGuiKey_End))
            Done = true;

        MSG Msg;
        while (::PeekMessage(&Msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&Msg);
            ::DispatchMessage(&Msg);
            if (Msg.message == WM_QUIT)
                Done = true;
        }
        if (Done)
            break;

        //  Handle traget size and style change
        if (Bd->bTargetChanged)
        {
            // Depending on your renderer(change if not dx9)
            g_d3dpp.BackBufferWidth     = Bd->TargetSize.right;
            g_d3dpp.BackBufferHeight    = Bd->TargetSize.bottom;

            //  Set new window size and pos
            ::SetWindowPos(hwnd, HWND_TOP, Bd->TargetClientToScreen.x, Bd->TargetClientToScreen.y, Bd->TargetSize.right, Bd->TargetSize.bottom, SWP_NOREDRAW);

            //  Depending on your renderer (change if not dx9)
            ResetDevice();

            //  Reset state because the change has been processed
            Bd->bTargetChanged          = false;
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWinHook_NewFrame();

        //  Should usually be done differently, this is just a poc. 
        //  More info on the methods can be found in the declaration of the func
        ImGui_ImplWinHook_UpdateGamepad();

        ImGui::NewFrame();

        //  Dont draw if target is not foreground
        if (Bd->bTargetForeground)
        {
            static int Counter = 0;

            ImGui::SetNextWindowSize({ 600.f, 400.f }, ImGuiCond_Once);
            ImGui::Begin("Input Test! (Press End To Exit)");
            {
                ImGui::Text("Test Text Input (Press Enter To Clear The Buffer)");
                std::string szInputName = "";
                if (ImGui::InputText("Input Text", &szInputName, ImGuiInputTextFlags_EnterReturnsTrue))
                    szInputName.clear();
                ImGui::Text(szInputName.c_str());
                ImGui::Dummy({ 0, 10 });

                ImGui::Text("Test Button Input");
                if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                   Counter++;
                ImGui::SameLine();
                ImGui::Text("Counter = %d", Counter);
                ImGui::Dummy({ 0, 10 });

                ImGui::Text("Test IsKeyDown Input By Holding Space:");
                if (ImGui::IsKeyDown(ImGuiKey_Space))
                    ImGui::Text("Space Is Being Held!");
                ImGui::Dummy({ 0, 10 });

                ImGui::Text("Test IsKeyReleased Input By Prsssing Tab:");
                static bool Toggle = false;

                if (ImGui::IsKeyReleased(ImGuiKey_Tab))
                    Toggle = !Toggle;

                if (Toggle)
                    ImGui::Text("Tab is toggled!");
                ImGui::Dummy({ 0, 10 });

                ImGui::Text("Test Scroll ListBox:");
                const char* Items[] = { "Item 1", "Item 2", "Item 3", "Item 4", "Item 5", "Item 6", "Item 7", "Item 8", "Item 9", "Item 10", "Item 11", "Item 12", "Item 13", "Item 14" };
                static int CurrItem = 1;
                ImGui::ListBox("###", &CurrItem, Items, IM_ARRAYSIZE(Items), 4);
                ImGui::Dummy({ 0, 10 });

                ImGui::Text("Mouse 4/5 Test");
                if (ImGui::IsMouseDown(3))
                    ImGui::Text("Mouse 4 Is Being Held!");

                if (ImGui::IsMouseDown(4))
                    ImGui::Text("Mouse 5 Is Being Held!");
                ImGui::Dummy({ 0, 10 });

                ImGui::Text("To test controller input hold X and move the left stick around!");

                ImGui::End();
            }
        }

        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
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
    ImGui_ImplWinHook_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

#if defined(REL_WITHDBG) || defined(_DEBUG)
    if (!DestroyConsole())
        return EXIT_FAILURE;
#endif

    return EXIT_SUCCESS;
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

// Find target functions

ULONG GetProcId(std::wstring_view szProcName)
{
    PROCESSENTRY32 ProcInfo;
    ProcInfo.dwSize = sizeof(ProcInfo);

    HANDLE ProcSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (ProcSnap == INVALID_HANDLE_VALUE)
        return 0;

    Process32First(ProcSnap, &ProcInfo);
    if (!szProcName.compare(ProcInfo.szExeFile))
    {
        CloseHandle(ProcSnap);
        return ProcInfo.th32ProcessID;
    }

    while (Process32Next(ProcSnap, &ProcInfo))
    {
        if (!szProcName.compare(ProcInfo.szExeFile))
        {
            CloseHandle(ProcSnap);
            return ProcInfo.th32ProcessID;
        }
    }

    CloseHandle(ProcSnap);
    return 0;
}

HWND GetWnd(ULONG Pid)
{
    std::pair<HWND, ULONG> Params = { 0, Pid };

    BOOL bResult = EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
    {
        auto pParams = (std::pair<HWND, ULONG>*)(lParam);

        ULONG  ProcId;
        if (GetWindowThreadProcessId(hwnd, &ProcId) && ProcId == pParams->second)
        {
            pParams->first = hwnd;
            return FALSE;
        }

        return TRUE;

    }, (LPARAM)&Params);

    if (!bResult && Params.first)
        return Params.first;

    return nullptr;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed                = TRUE;
    g_d3dpp.SwapEffect              = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat        = D3DFMT_A8R8G8B8;
    g_d3dpp.BackBufferWidth         = TargetRect.right;
    g_d3dpp.BackBufferHeight        = TargetRect.bottom;
    g_d3dpp.EnableAutoDepthStencil  = TRUE;
    g_d3dpp.AutoDepthStencilFormat  = D3DFMT_D16;
    g_d3dpp.PresentationInterval    = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval  = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
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