; ===========================================
; CONFIGURATION - Change this to remap the toggle key
; ===========================================
TOGGLE_KEY := "``"  ; Change this to any key you want (e.g., "F1", "CapsLock", "Space", "Tab", etc.)

; ===========================================
; Virtual Desktop Toggle Script
; ===========================================

; --- Load VirtualDesktopAccessor.dll ---
VDA_PATH := A_ScriptDir . "\VirtualDesktopAccessor.dll"
hVirtualDesktopAccessor := DllCall("LoadLibrary", "Str", VDA_PATH, "Ptr")

; --- Get function pointers from the DLL ---
GetDesktopCountProc := DllCall("GetProcAddress", "Ptr", hVirtualDesktopAccessor, "AStr", "GetDesktopCount", "Ptr")
GetCurrentDesktopNumberProc := DllCall("GetProcAddress", "Ptr", hVirtualDesktopAccessor, "AStr", "GetCurrentDesktopNumber", "Ptr")
GoToDesktopNumberProc := DllCall("GetProcAddress", "Ptr", hVirtualDesktopAccessor, "AStr", "GoToDesktopNumber", "Ptr")

; --- Function to get total number of desktops ---
GetDesktopCount() {
    global GetDesktopCountProc
    return DllCall(GetDesktopCountProc, "Int")
}

; --- Function to switch to a specific desktop ---
GoToDesktopNumber(num) {
    global GoToDesktopNumberProc
    DllCall(GoToDesktopNumberProc, "Int", num, "Int")
}

; --- Function to get the current desktop number ---
GetCurrentDesktopNumber() {
    global GetCurrentDesktopNumberProc
    return DllCall(GetCurrentDesktopNumberProc, "Int")
}

; --- Toggle state ---
Toggle := 1

; --- Dynamic hotkey creation ---
Hotkey, %TOGGLE_KEY%, ToggleDesktop
return

; --- Toggle function ---
ToggleDesktop:
    current := GetCurrentDesktopNumber()
    last := GetDesktopCount() - 1

    if (Toggle = 1) {
        if (current > 0)
            GoToDesktopNumber(current - 1)
        Toggle := 0
    } else {
        if (current < last)
            GoToDesktopNumber(current + 1)
        Toggle := 1
    }
return