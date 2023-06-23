
 <p align="center"> 
    <a href="#" target="_blank"> <img alt="Lines of code" src="https://img.shields.io/tokei/lines/github/nexusflipp/imgui_winhook"> </a>
    <a href="#" target="_blank"> <img src="https://img.shields.io/github/issues/nexusflipp/imgui_winhook"/> </a>
    <a href="#" target="_blank"> <img src="https://img.shields.io/github/languages/top/nexusflipp/imgui_winhook"/> </a> 
    <a href="#" target="_blank"> <img src="https://img.shields.io/github/last-commit/nexusflipp/imgui_winhook"/> </a> 
    <a href="#" target="_blank"> <img src="https://img.shields.io/github/languages/code-size/nexusflipp/imgui_winhook"/> </a> 
</p>

# IMGUI WINHOOK BACKEND

This project is an alternative Windows platform backend for the Dear ImGui library.
Inputs are handled through global low level windows hooks. If you encounter any mouse lag please read: [Mouse Lag](#Mouse-Lag)

This enables input handling for overlays and other windows that are not able to process messages through the WndProc callback (WS_EX_LAYERED).

There are two examples in the examples directory:

*   A target window that is able to change window settings.
*   An overlay which can receive, handle and test events.

A more detailed list of features can be found below. Note that these are POCs (Not a lot of error handling/input validating).

## Features Of The Backend

*   Handle keyboard events and passes them to the event handler.
*   Handle mouse events and passes them to the mouse event handler.
*   Handle gamepad events through XInput and passes them to the event handler.
*   Sanitize the keyboard input and send the sanitized input to the character input handler.
*   The sanitizer detects shift and caps lock events and then returns as lower or upper case.
*   The sanitizer detects numpad keys and converts them into ascii values, which will be also passed into the character input.
*   Detects changes in the target window and adapts to them. Window position, size and style.
*   Stops handling input if the target window is not foreground.
*   Calculates the mouse position, and adapts it to the target window.
*   Can handle mouse events through a debug mouse handler to prevent mouse freezing: [More Information](#Mouse-Lag)


## Features Of The Example Target

*   Change window style.
*   Change window size.
*   Render with dx9 and ImGui.
*   Debug console creation.


## Features Of The Example Overlay

*   Get the process id of a given process.
*   Get the window of a given process.
*   Handle keyboard events.
*   Handle mouse events.
*   Handle gamepad events.
*   Reacts and adapts to the target window position, size and style.
*   Only renders when the target window is foreground.
*   Only handles events/inputs when the target window is foreground.
*   Adapts dx9 to handle resize events.
*   ImGui input test window.
*   Render with dx9 and ImGui.
*   Debug console creation.


## Important Notes

*   Do not use this backend together with the normal ImGui Windows backend. This is a replacement for it!
*   This is not compatible with all ImGui versions due to a drastic update of the key handler (Tested on v1.89.6).
*   You may have to expand the input sanitizer to your liking.
*   The IMGUI_IMPL_WINHOOK_DISABLE_GAMEPAD define can be used to disable XInput completely.
*   XInput should not be handled like in the POC, more information about this can be found in the source code.


## Mouse Lag

I have received numerous messages from people asking why their mouse lags when debugging with this backend.

This lag is entirely normal because these hooks are global. When you hit a breakpoint, the system is usually unable to capture any additional mouse events. The operating system detects this issue and removes our hook when it becomes too slow (in our case, when we hit a breakpoint). Consequently, the mouse starts to lag.

**To facilitate debugging for users, I have implemented a debug mouse handler.** This handler utilizes GetCursorPos instead of messages.

**The debug mouse handler is automatically enabled in debug mode. If you want to disable it in debug mode, use IMGUI_IMPL_WINHOOK_DISABLE_DEBUG_HANDLER. On the other hand, the debug mouse handler is automatically disabled in release mode. If you wish to enable it in release mode, use IMGUI_IMPL_WINHOOK_ENABLE_DEBUG_HANDLER.**

Please note that the debug handler only manages mouse movement and left mouse button clicks.


## Credits
*   [Omar](https://github.com/ocornut): For the Dear ImGui library.
*   [Reactos](https://reactos.org/): For awesome insights into win internals.
*   [Microsoft](https://learn.microsoft.com/en-us/docs/): For their extensive WinAPI documentation and XInput.
