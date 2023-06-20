#include "imgui_impl_winhook.h"
#include "imgui.h"
#include "VKDefs.hh"
#include <Windows.h>

#define REINTER                         reinterpret_cast
#define STATICCA                        static_cast
#define GET_WHEEL_DELTA(mouseData)      ((short)HIWORD(mouseData))
//  There is no distinct VK_xxx for keypad enter, instead it is VK_RETURN + KF_EXTENDED, we assign it an arbitrary value to make code more readable (VK_ codes go up to 255)
#define IM_VK_KEYPAD_ENTER      (VK_RETURN + 256)

// I use this to sanitize the input into AddInputCharacter, since the keyboard hook does not handle WM_CHAR
const DWORD InputSanitizer(const DWORD Key, const bool bCaps, const bool bShift)
{
    switch (Key)
    {

    //  Checks if any alpha key is pressed.
    //  Returns upper or lowercase depending on the parameters.
    case VK_A:
    case VK_B:
    case VK_C:
    case VK_D:
    case VK_E:
    case VK_F:
    case VK_G:
    case VK_H:
    case VK_I:
    case VK_J:
    case VK_K:
    case VK_L:
    case VK_M:
    case VK_N:
    case VK_O:
    case VK_P:
    case VK_Q:
    case VK_R:
    case VK_S:
    case VK_T:
    case VK_U:
    case VK_V:
    case VK_W:
    case VK_X:
    case VK_Y:
    case VK_Z:
    {
        if (bCaps || bShift)
            return Key;
        else
            return Key + ('a' - 'A');
    }

    //  Keys are already ascii
    case VK_0:
    case VK_1:
    case VK_2:
    case VK_3:
    case VK_4:
    case VK_5:
    case VK_6:
    case VK_7:
    case VK_8:
    case VK_9:
        return Key;

    //  Converts numpad keys to ascii numbers
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
        return Key - VK_NUMPAD0 + '0';

    //  Ignore all other keys
    default:
        return 0;
    }
}

//  Map VK_xxx to ImGuiKey_xxx.
static ImGuiKey ImGui_ImplWinHook_VirtualKeyToImGuiKey(DWORD vkCode)
{
    switch (vkCode)
    {
    case VK_TAB:                return ImGuiKey_Tab;
    case VK_LEFT:               return ImGuiKey_LeftArrow;
    case VK_RIGHT:              return ImGuiKey_RightArrow;
    case VK_UP:                 return ImGuiKey_UpArrow;
    case VK_DOWN:               return ImGuiKey_DownArrow;
    case VK_PRIOR:              return ImGuiKey_PageUp;
    case VK_NEXT:               return ImGuiKey_PageDown;
    case VK_HOME:               return ImGuiKey_Home;
    case VK_END:                return ImGuiKey_End;
    case VK_INSERT:             return ImGuiKey_Insert;
    case VK_DELETE:             return ImGuiKey_Delete;
    case VK_BACK:               return ImGuiKey_Backspace;
    case VK_SPACE:              return ImGuiKey_Space;
    case VK_RETURN:             return ImGuiKey_Enter;
    case VK_ESCAPE:             return ImGuiKey_Escape;
    case VK_OEM_7:              return ImGuiKey_Apostrophe;
    case VK_OEM_COMMA:          return ImGuiKey_Comma;
    case VK_OEM_MINUS:          return ImGuiKey_Minus;
    case VK_OEM_PERIOD:         return ImGuiKey_Period;
    case VK_OEM_2:              return ImGuiKey_Slash;
    case VK_OEM_1:              return ImGuiKey_Semicolon;
    case VK_OEM_PLUS:           return ImGuiKey_Equal;
    case VK_OEM_4:              return ImGuiKey_LeftBracket;
    case VK_OEM_5:              return ImGuiKey_Backslash;
    case VK_OEM_6:              return ImGuiKey_RightBracket;
    case VK_OEM_3:              return ImGuiKey_GraveAccent;
    case VK_CAPITAL:            return ImGuiKey_CapsLock;
    case VK_SCROLL:             return ImGuiKey_ScrollLock;
    case VK_NUMLOCK:            return ImGuiKey_NumLock;
    case VK_SNAPSHOT:           return ImGuiKey_PrintScreen;
    case VK_PAUSE:              return ImGuiKey_Pause;
    case VK_NUMPAD0:            return ImGuiKey_Keypad0;
    case VK_NUMPAD1:            return ImGuiKey_Keypad1;
    case VK_NUMPAD2:            return ImGuiKey_Keypad2;
    case VK_NUMPAD3:            return ImGuiKey_Keypad3;
    case VK_NUMPAD4:            return ImGuiKey_Keypad4;
    case VK_NUMPAD5:            return ImGuiKey_Keypad5;
    case VK_NUMPAD6:            return ImGuiKey_Keypad6;
    case VK_NUMPAD7:            return ImGuiKey_Keypad7;
    case VK_NUMPAD8:            return ImGuiKey_Keypad8;
    case VK_NUMPAD9:            return ImGuiKey_Keypad9;
    case VK_DECIMAL:            return ImGuiKey_KeypadDecimal;
    case VK_DIVIDE:             return ImGuiKey_KeypadDivide;
    case VK_MULTIPLY:           return ImGuiKey_KeypadMultiply;
    case VK_SUBTRACT:           return ImGuiKey_KeypadSubtract;
    case VK_ADD:                return ImGuiKey_KeypadAdd;
    case IM_VK_KEYPAD_ENTER:    return ImGuiKey_KeypadEnter;
    case VK_LSHIFT:             return ImGuiKey_LeftShift;
    case VK_LCONTROL:           return ImGuiKey_LeftCtrl;
    case VK_LMENU:              return ImGuiKey_LeftAlt;
    case VK_LWIN:               return ImGuiKey_LeftSuper;
    case VK_RSHIFT:             return ImGuiKey_RightShift;
    case VK_RCONTROL:           return ImGuiKey_RightCtrl;
    case VK_RMENU:              return ImGuiKey_RightAlt;
    case VK_RWIN:               return ImGuiKey_RightSuper;
    case VK_APPS:               return ImGuiKey_Menu;
    case VK_0:                  return ImGuiKey_0;
    case VK_1:                  return ImGuiKey_1;
    case VK_2:                  return ImGuiKey_2;
    case VK_3:                  return ImGuiKey_3;
    case VK_4:                  return ImGuiKey_4;
    case VK_5:                  return ImGuiKey_5;
    case VK_6:                  return ImGuiKey_6;
    case VK_7:                  return ImGuiKey_7;
    case VK_8:                  return ImGuiKey_8;
    case VK_9:                  return ImGuiKey_9;
    case VK_A:                  return ImGuiKey_A;
    case VK_B:                  return ImGuiKey_B;
    case VK_C:                  return ImGuiKey_C;
    case VK_D:                  return ImGuiKey_D;
    case VK_E:                  return ImGuiKey_E;
    case VK_F:                  return ImGuiKey_F;
    case VK_G:                  return ImGuiKey_G;
    case VK_H:                  return ImGuiKey_H;
    case VK_I:                  return ImGuiKey_I;
    case VK_J:                  return ImGuiKey_J;
    case VK_K:                  return ImGuiKey_K;
    case VK_L:                  return ImGuiKey_L;
    case VK_M:                  return ImGuiKey_M;
    case VK_N:                  return ImGuiKey_N;
    case VK_O:                  return ImGuiKey_O;
    case VK_P:                  return ImGuiKey_P;
    case VK_Q:                  return ImGuiKey_Q;
    case VK_R:                  return ImGuiKey_R;
    case VK_S:                  return ImGuiKey_S;
    case VK_T:                  return ImGuiKey_T;
    case VK_U:                  return ImGuiKey_U;
    case VK_V:                  return ImGuiKey_V;
    case VK_W:                  return ImGuiKey_W;
    case VK_X:                  return ImGuiKey_X;
    case VK_Y:                  return ImGuiKey_Y;
    case VK_Z:                  return ImGuiKey_Z;
    case VK_F1:                 return ImGuiKey_F1;
    case VK_F2:                 return ImGuiKey_F2;
    case VK_F3:                 return ImGuiKey_F3;
    case VK_F4:                 return ImGuiKey_F4;
    case VK_F5:                 return ImGuiKey_F5;
    case VK_F6:                 return ImGuiKey_F6;
    case VK_F7:                 return ImGuiKey_F7;
    case VK_F8:                 return ImGuiKey_F8;
    case VK_F9:                 return ImGuiKey_F9;
    case VK_F10:                return ImGuiKey_F10;
    case VK_F11:                return ImGuiKey_F11;
    case VK_F12:                return ImGuiKey_F12;

    default:                    return ImGuiKey_None;
    }
}

static void ImGui_ImplWinHook_AddKeyEvent(ImGuiKey key, bool down, int native_keycode, int native_scancode = -1)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(key, down);
    io.SetKeyEventNativeData(key, native_keycode, native_scancode); // To support legacy indexing (<1.87 user code)
    IM_UNUSED(native_scancode);
}

static bool IsVkDown(int Vk)
{
    return (::GetKeyState(Vk) & 0x8000) != 0;
}

static bool IsVkToggled(int Vk)
{
    return (::GetKeyState(Vk) & 0x0001) != 0;
}

static void ImGui_ImplWinHook_ProcessKeyEventsWorkarounds()
{
    // Left & right Shift keys: when both are pressed together, Windows tend to not generate the WM_KEYUP event for the first released one.
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) && !IsVkDown(VK_LSHIFT))
        ImGui_ImplWinHook_AddKeyEvent(ImGuiKey_LeftShift, false, VK_LSHIFT);

    if (ImGui::IsKeyDown(ImGuiKey_RightShift) && !IsVkDown(VK_RSHIFT))
        ImGui_ImplWinHook_AddKeyEvent(ImGuiKey_RightShift, false, VK_RSHIFT);

    // Sometimes WM_KEYUP for Win key is not passed down to the app (e.g. for Win+V on some setups, according to GLFW).
    if (ImGui::IsKeyDown(ImGuiKey_LeftSuper) && !IsVkDown(VK_LWIN))
        ImGui_ImplWinHook_AddKeyEvent(ImGuiKey_LeftSuper, false, VK_LWIN);

    if (ImGui::IsKeyDown(ImGuiKey_RightSuper) && !IsVkDown(VK_RWIN))
        ImGui_ImplWinHook_AddKeyEvent(ImGuiKey_RightSuper, false, VK_RWIN);
}

LRESULT hkLowLvlMouse(int nCode, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWinHook_Data* Bd  = ImGui_ImplWinHook_GetBackendData();

    //  Dont handle input when out target is not foreground
    if (!Bd->bTargetForeground)
        return CallNextHookEx(Bd->MouseHook, nCode, wParam, lParam);

    if (nCode >= HC_ACTION)
    {
        ImGuiIO&            Io          = ImGui::GetIO();
        PMSLLHOOKSTRUCT     pMouseData  = REINTER<PMSLLHOOKSTRUCT>(lParam);

        switch (wParam)
        {

        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
        {
            Io.MousePos.x = STATICCA<float>(pMouseData->pt.x - Bd->TargetClientToScreen.x);
            Io.MousePos.y = STATICCA<float>(pMouseData->pt.y - Bd->TargetClientToScreen.y);
            break;
        }

        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
            Io.AddMouseButtonEvent(0, true);
            break;

        case WM_LBUTTONUP:
            Io.AddMouseButtonEvent(0, false);
            break;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
            Io.AddMouseButtonEvent(1, true);
            break;

        case WM_RBUTTONUP:
            Io.AddMouseButtonEvent(1, false);
            break;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
            Io.AddMouseButtonEvent(2, true);
            break;

        case WM_MBUTTONUP:
            Io.AddMouseButtonEvent(2, false);
            break;

        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        {
            //  Mouse Btn 4 or 5
            int Button = (HIWORD(pMouseData->mouseData) == XBUTTON1) ? 3 : 4;

            Io.AddMouseButtonEvent(Button, true);
            break;
        }

        case WM_XBUTTONUP:
        {
            //  Mouse Btn 4 or 5
            int Button = (HIWORD(pMouseData->mouseData) == XBUTTON1) ? 3 : 4;

            Io.AddMouseButtonEvent(Button, false);
            break;
        }

        case WM_MOUSEWHEEL:
            Io.AddMouseWheelEvent(0.0f, (float)GET_WHEEL_DELTA(pMouseData->mouseData) / (float)WHEEL_DELTA);
            break;

        case WM_MOUSEHWHEEL:
            Io.AddMouseWheelEvent(-(float)GET_WHEEL_DELTA(pMouseData->mouseData) / (float)WHEEL_DELTA, 0.0f);
            break;

        default:
            break;
        }
    }

    return CallNextHookEx(Bd->MouseHook, nCode, wParam, lParam);
}

LRESULT hkLowLvlKeyboard(int nCode, WPARAM wParam, LPARAM lParam)
{
    ImGui_ImplWinHook_Data* Bd  = ImGui_ImplWinHook_GetBackendData();

    //  Dont handle input when out target is not foreground
    if (!Bd->bTargetForeground)
        return CallNextHookEx(Bd->KeyboardHook, nCode, wParam, lParam);

    if (nCode >= HC_ACTION)
    {
        ImGuiIO&            Io          = ImGui::GetIO();
        PKBDLLHOOKSTRUCT    pKeyData    = REINTER<PKBDLLHOOKSTRUCT>(lParam);

        switch (wParam)
        {

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            DWORD VkKey = pKeyData->vkCode;
            // (keypad enter doesn't have its own... VK_RETURN with KF_EXTENDED flag means keypad enter, see IM_VK_KEYPAD_ENTER definition for details, it is mapped to ImGuiKey_KeyPadEnter.)
            if ((wParam == VK_RETURN) && (pKeyData->flags & LLKHF_EXTENDED))
                VkKey = IM_VK_KEYPAD_ENTER;

            ImGuiKey ImKey = ImGui_ImplWinHook_VirtualKeyToImGuiKey(VkKey);
            if (ImKey == ImGuiKey_None)
                break;

            ImGui_ImplWinHook_AddKeyEvent(ImKey, true, VkKey, pKeyData->scanCode);

            //  I sanitize the input into AddInputCharacter, since the keyboard hook does not handle WM_CHAR 
            //  Adjust the sanitizer to your liking, right now it only lets alphabet and num keys through.
            const DWORD InputKey = InputSanitizer(VkKey, IsVkToggled(VK_CAPITAL), IsVkDown(VK_SHIFT));
            if (InputKey == 0)
                break;

            if (::IsWindowUnicode(Bd->OwnWnd))
                Io.AddInputCharacterUTF16((ImWchar16)InputKey);
            else
            {
                wchar_t wch = 0;
                ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&InputKey, 1, &wch, 1);
                Io.AddInputCharacter(wch);
            }

            break;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            DWORD VkKey = pKeyData->vkCode;
            // (keypad enter doesn't have its own... VK_RETURN with KF_EXTENDED flag means keypad enter, see IM_VK_KEYPAD_ENTER definition for details, it is mapped to ImGuiKey_KeyPadEnter.)
            if ((wParam == VK_RETURN) && (pKeyData->flags & LLKHF_EXTENDED))
                VkKey = IM_VK_KEYPAD_ENTER;

            ImGuiKey ImKey = ImGui_ImplWinHook_VirtualKeyToImGuiKey(VkKey);
            if (ImKey == ImGuiKey_None)
                break;

            ImGui_ImplWinHook_AddKeyEvent(ImKey, false, VkKey, pKeyData->scanCode);

            break;
        }

        default:
            break;
        }
    }

    return CallNextHookEx(Bd->KeyboardHook, nCode, wParam, lParam);
}

IMGUI_IMPL_API bool ImGui_ImplWinHook_Init(void* OwnWnd, void* TargetWnd)
{
	ImGuiIO& Io = ImGui::GetIO();
	IM_ASSERT(Io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

	INT64 PerfFrequency, PerfCounter;
	if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFrequency))
		return false;
	if (!::QueryPerformanceCounter((LARGE_INTEGER*)&PerfCounter))
		return false;

	// Setup backend capabilities flags
	ImGui_ImplWinHook_Data* Bd  = IM_NEW(ImGui_ImplWinHook_Data)();
	Io.BackendPlatformUserData  = (void*)Bd;
	Io.BackendPlatformName      = "imgui_impl_winhook";

    // Cache target size and pos in order to handle if it changed
    ::GetClientRect((HWND)TargetWnd, &Bd->TargetSize);
    ::ClientToScreen((HWND)TargetWnd, &Bd->TargetClientToScreen);

    //  Initial display size setup
    Io.DisplaySize = ImVec2((float)(Bd->TargetSize.right - Bd->TargetSize.left), (float)(Bd->TargetSize.bottom - Bd->TargetSize.top));

    Bd->OwnWnd          = (HWND)OwnWnd;
	Bd->TargetWnd       = (HWND)TargetWnd;

	Bd->TicksPerSecond  = PerfFrequency;
	Bd->Time            = PerfCounter;

    Bd->MouseHook       = SetWindowsHookExA(WH_MOUSE_LL, hkLowLvlMouse, nullptr, 0);
    IM_ASSERT(Bd->MouseHook != NULL && "Mouse hook failed!");

    Bd->KeyboardHook    = SetWindowsHookExA(WH_KEYBOARD_LL, hkLowLvlKeyboard, nullptr, 0);
    IM_ASSERT(Bd->KeyboardHook != NULL && "Keyboard hook failed!");

    //  Set platform dependent data in viewport
    ImGui::GetMainViewport()->PlatformHandleRaw = (void*)OwnWnd;

    //  Dynamically load XInput library
#ifndef IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD

    const char* xinput_dll_names[] =
    {
        "xinput1_4.dll",   //  Windows 8+
        "xinput1_3.dll",   //  DirectX SDK
        "xinput9_1_0.dll", //  Windows Vista, Windows 7
        "xinput1_2.dll",   //  DirectX SDK
        "xinput1_1.dll"    //  DirectX SDK
    };
    for (int n = 0; n < IM_ARRAYSIZE(xinput_dll_names); n++)
        if (HMODULE dll = ::LoadLibraryA(xinput_dll_names[n]))
        {
            Bd->XInputDLL               = dll;
            Bd->XInputGetCapabilities   = (PFN_XInputGetCapabilities)::GetProcAddress(dll, "XInputGetCapabilities");
            Bd->XInputGetState          = (PFN_XInputGetState)::GetProcAddress(dll, "XInputGetState");
            break;
        }

#endif //  IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD

	return true;
}

IMGUI_IMPL_API void ImGui_ImplWinHook_Shutdown()
{
	ImGui_ImplWinHook_Data* Bd = ImGui_ImplWinHook_GetBackendData();
	IM_ASSERT(Bd != nullptr && "No platform backend to shutdown, or already shutdown?");
	ImGuiIO& Io = ImGui::GetIO();

    UnhookWindowsHookEx(Bd->MouseHook);
    UnhookWindowsHookEx(Bd->KeyboardHook);

        //  Unload XInput library
#ifndef IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD
    if (Bd->XInputDLL)
        ::FreeLibrary(Bd->XInputDLL);
#endif  //  IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD

	Io.BackendPlatformName      = nullptr;
	Io.BackendPlatformUserData  = nullptr;
    Io.BackendFlags             &= ~ImGuiBackendFlags_HasGamepad;

	IM_DELETE(Bd);
}

bool AreRectsEqual(const RECT& Rect1, const RECT& Rect2)
{
    return  Rect1.left      == Rect2.left       &&
            Rect1.top       == Rect2.top        &&
            Rect1.right     == Rect2.right      &&
            Rect1.bottom    == Rect2.bottom;
}

bool ArePointsEqual(const POINT& Point1, const POINT& Point2)
{
    return  Point1.x        == Point2.x         && 
            Point1.y        == Point2.y;
}

IMGUI_IMPL_API void ImGui_ImplWinHook_NewFrame()
{
    ImGuiIO&                Io = ImGui::GetIO();
    ImGui_ImplWinHook_Data* Bd = ImGui_ImplWinHook_GetBackendData();
    IM_ASSERT(Bd != nullptr && "Did you call ImGui_ImplWinHook_Init()?");

    RECT    Rect    = { 0, 0, 0, 0 };
    POINT   Point   = { 0, 0 };

    ::GetClientRect(Bd->TargetWnd, &Rect);
    ::ClientToScreen(Bd->TargetWnd, &Point);

    //  Target size or style changed, save new state and indicate change
    if (!AreRectsEqual(Bd->TargetSize, Rect) || !ArePointsEqual(Bd->TargetClientToScreen, Point))
    {
        Bd->TargetSize              = Rect;
        Bd->TargetClientToScreen    = Point;
        Io.DisplaySize              = ImVec2((float)(Bd->TargetSize.right - Bd->TargetSize.left), (float)(Bd->TargetSize.bottom - Bd->TargetSize.top));

        Bd->bTargetChanged          = true;
    }

    // Setup time step
    INT64 CurrTime  = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&CurrTime);
    Io.DeltaTime    = (float)(CurrTime - Bd->Time) / Bd->TicksPerSecond;
    Bd->Time        = CurrTime;

    // Process workarounds for known Windows key handling issues
    ImGui_ImplWinHook_ProcessKeyEventsWorkarounds();

    HWND ActiveHwnd = GetForegroundWindow();
    if (ActiveHwnd == Bd->TargetWnd)
        Bd->bTargetForeground = true;
    else
        Bd->bTargetForeground = false;
}

IMGUI_IMPL_API void ImGui_ImplWinHook_DetectGamepad()
{
#ifndef IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD

    ImGuiIO&                Io      = ImGui::GetIO();
    ImGui_ImplWinHook_Data* Bd      = ImGui_ImplWinHook_GetBackendData();

    //  Gamepad check
    XINPUT_CAPABILITIES     Caps    = {};
    if (Bd->XInputGetCapabilities == nullptr || Bd->XInputGetCapabilities(0, XINPUT_FLAG_GAMEPAD, &Caps) != ERROR_SUCCESS)
    {
        Bd->HasGamepad      = false;
        Io.BackendFlags     &= ~ImGuiBackendFlags_HasGamepad;
        return;
    }

    Bd->HasGamepad = true;

#endif // IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD
}

// Gamepad navigation mapping
IMGUI_IMPL_API void ImGui_ImplWinHook_UpdateGamepad()
{
#ifndef IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD

    ImGuiIO& Io = ImGui::GetIO();
    ImGui_ImplWinHook_Data* Bd = ImGui_ImplWinHook_GetBackendData();

    if (!Bd->HasGamepad || Bd->XInputGetState == nullptr || Bd->XInputGetState(0, &Bd->XinputState) != ERROR_SUCCESS)
    {
        Bd->HasGamepad = false;
        Io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
        return;
    }

    Io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

#define IM_SATURATE(V)                          (V < 0.0f ? 0.0f : V > 1.0f ? 1.0f : V)
#define MAP_BUTTON(KEY_NO, BUTTON_ENUM)         { Io.AddKeyEvent(KEY_NO, (Bd->XinputState.Gamepad.wButtons & BUTTON_ENUM) != 0); }
#define MAP_ANALOG(KEY_NO, VALUE, V0, V1)       { float vn = (float)(VALUE - V0) / (float)(V1 - V0); Io.AddKeyAnalogEvent(KEY_NO, vn > 0.10f, IM_SATURATE(vn)); }

    MAP_BUTTON(ImGuiKey_GamepadStart,           XINPUT_GAMEPAD_START);
    MAP_BUTTON(ImGuiKey_GamepadBack,            XINPUT_GAMEPAD_BACK);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,        XINPUT_GAMEPAD_X);
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,       XINPUT_GAMEPAD_B);
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,          XINPUT_GAMEPAD_Y);
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,        XINPUT_GAMEPAD_A);
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,        XINPUT_GAMEPAD_DPAD_LEFT);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight,       XINPUT_GAMEPAD_DPAD_RIGHT);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,          XINPUT_GAMEPAD_DPAD_UP);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,        XINPUT_GAMEPAD_DPAD_DOWN);
    MAP_BUTTON(ImGuiKey_GamepadL1,              XINPUT_GAMEPAD_LEFT_SHOULDER);
    MAP_BUTTON(ImGuiKey_GamepadR1,              XINPUT_GAMEPAD_RIGHT_SHOULDER);
    MAP_ANALOG(ImGuiKey_GamepadL2,              Bd->XinputState.Gamepad.bLeftTrigger,       XINPUT_GAMEPAD_TRIGGER_THRESHOLD,       255);
    MAP_ANALOG(ImGuiKey_GamepadR2,              Bd->XinputState.Gamepad.bRightTrigger,      XINPUT_GAMEPAD_TRIGGER_THRESHOLD,       255);
    MAP_BUTTON(ImGuiKey_GamepadL3,              XINPUT_GAMEPAD_LEFT_THUMB);
    MAP_BUTTON(ImGuiKey_GamepadR3,              XINPUT_GAMEPAD_RIGHT_THUMB);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,      Bd->XinputState.Gamepad.sThumbLX,           -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,    -32768);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight,     Bd->XinputState.Gamepad.sThumbLX,           +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,    +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,        Bd->XinputState.Gamepad.sThumbLY,           +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,    +32767);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,      Bd->XinputState.Gamepad.sThumbLY,           -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,    -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,      Bd->XinputState.Gamepad.sThumbRX,           -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,    -32768);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight,     Bd->XinputState.Gamepad.sThumbRX,           +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,    +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,        Bd->XinputState.Gamepad.sThumbRY,           +XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,    +32767);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,      Bd->XinputState.Gamepad.sThumbRY,           -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE,    -32768);

#undef MAP_BUTTON
#undef MAP_ANALOG

#endif // #ifndef IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD
}