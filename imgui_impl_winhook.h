#pragma once
#include <Windows.h>
#include "imgui.h"      // IMGUI_IMPL_API

// Configuration flags to add in your imconfig.h file:
//#define IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD              // Disable gamepad support. This was meaningful before <1.81 but we now load XInput dynamically so the option is now less relevant.

// Using XInput for gamepad (will load DLL dynamically)
#ifndef IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD
#include <xinput.h>
typedef DWORD(WINAPI* PFN_XInputGetCapabilities)(DWORD, DWORD, XINPUT_CAPABILITIES*);
typedef DWORD(WINAPI* PFN_XInputGetState)(DWORD, XINPUT_STATE*);
#endif

struct ImGui_ImplWinHook_Data
{
	HWND				OwnWnd;

	HWND				TargetWnd;
	RECT				TargetSize;
	POINT				TargetClientToScreen;

	bool                bTargetForeground;
	bool				bTargetChanged;

	HHOOK				MouseHook;
	HHOOK				KeyboardHook;

	INT64				Time;
	INT64				TicksPerSecond;

#ifndef IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD

	bool                        HasGamepad;
	HMODULE                     XInputDLL;
	XINPUT_STATE				XinputState;
	PFN_XInputGetCapabilities   XInputGetCapabilities;
	PFN_XInputGetState          XInputGetState;

#endif  //  #ifndef IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD

	ImGui_ImplWinHook_Data() { memset((void*)this, 0, sizeof(*this)); }
};

//  Dont call before ImGui_ImplWinHook_Init
IMGUI_IMPL_API static ImGui_ImplWinHook_Data* ImGui_ImplWinHook_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplWinHook_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

IMGUI_IMPL_API bool		ImGui_ImplWinHook_Init				(void* OwnWnd, void* TargetWnd);
IMGUI_IMPL_API void		ImGui_ImplWinHook_Shutdown			();
IMGUI_IMPL_API void		ImGui_ImplWinHook_NewFrame			();

//	Either we queue these functions every now and then to reduce draw time, or we only call it if the user wants it.
//  Methods: Use multithreading, QueueTimer, a blank msg wnd to intercept WM_DEVICECHANGE, hijack msgs from another wnd or manual call.
IMGUI_IMPL_API void		ImGui_ImplWinHook_DetectGamepad		();
IMGUI_IMPL_API void		ImGui_ImplWinHook_UpdateGamepad		();