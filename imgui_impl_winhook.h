#pragma once
#include <Windows.h>
#include "imgui.h"      // IMGUI_IMPL_API

#ifndef _DEBUG
#define _NDEBUG
#endif

//  Disable or enable debug mouse handler
//	
//	The debug mouse handler is automatically enabled in debug mode. To disable it in debug mode use IMGUI_IMPL_WINHOOK_DISABLE_DEBUG_HANDLER
//	The debug mouse handler is automatically disabled in release mode. To enable it in release mode use IMGUI_IMPL_WINHOOK_ENABLE_DEBUG_HANDLER
// 
//#define IMGUI_IMPL_WINHOOK_DISABLE_DEBUG_HANDLER
//#define IMGUI_IMPL_WINHOOK_ENABLE_DEBUG_HANDLER


//  Disable gamepad support.
//#define IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD

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

#if !defined(IMGUI_IMPL_WINHOOK_ENABLE_DEBUG_HANDLER) && (defined(_NDEBUG) || defined(IMGUI_IMPL_WINHOOK_DISABLE_DEBUG_HANDLER))
	HHOOK				MouseHook;
#else
	POINT				CursorPos;
#endif

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