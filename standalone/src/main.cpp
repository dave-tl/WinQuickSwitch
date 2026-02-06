// =============================================================================
// main.cpp - WinQuickSwitch Standalone Application
//
// A lightweight Windows system tray application that provides instant virtual
// desktop switching via a configurable hotkey. No external dependencies.
//
// Features:
//   - Single configurable hotkey to toggle between adjacent virtual desktops
//   - No animation delay (direct COM-based switching)
//   - System tray icon with right-click menu
//   - Optional "Start with Windows" auto-launch
//   - INI-based configuration
//   - Falls back to keyboard simulation if COM interfaces unavailable
// =============================================================================

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <stdio.h>

#include "resource.h"
#include "virtual_desktop.h"

// =============================================================================
// Globals
// =============================================================================
static HINSTANCE        g_hInstance     = NULL;
static HWND             g_hWnd         = NULL;
static NOTIFYICONDATAW  g_nid          = {};
static VirtualDesktopManager g_vdm;

// Configuration
static UINT  g_hotkeyVk       = VK_OEM_3;  // Backtick (`)
static UINT  g_hotkeyMod      = 0;          // No modifier by default
static BOOL  g_startWithWindows = FALSE;

// Registry key for auto-start
static const WCHAR* REG_RUN_KEY  = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
static const WCHAR* REG_APP_NAME = L"WinQuickSwitch";

// Config file path
static WCHAR g_iniPath[MAX_PATH] = {};

// =============================================================================
// Forward declarations
// =============================================================================
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void CreateTrayIcon(HWND hWnd);
static void RemoveTrayIcon();
static void ShowTrayMenu(HWND hWnd);
static void LoadConfig();
static void ToggleStartWithWindows();
static BOOL IsStartWithWindowsEnabled();
static UINT VkFromName(const WCHAR* name);
static HICON CreateDesktopSwitchIcon(int size);

// =============================================================================
// Helper: Parse virtual key code from string name
// =============================================================================
static UINT VkFromName(const WCHAR* name) {
    if (!name || !name[0]) return 0;

    // Single character keys
    if (wcslen(name) == 1) {
        WCHAR ch = towupper(name[0]);
        if (ch >= L'A' && ch <= L'Z') return (UINT)ch;
        if (ch >= L'0' && ch <= L'9') return (UINT)ch;
        if (ch == L'`')  return VK_OEM_3;
        if (ch == L'-')  return VK_OEM_MINUS;
        if (ch == L'=')  return VK_OEM_PLUS;
        if (ch == L'[')  return VK_OEM_4;
        if (ch == L']')  return VK_OEM_6;
        if (ch == L'\\') return VK_OEM_5;
        if (ch == L';')  return VK_OEM_1;
        if (ch == L'\'') return VK_OEM_7;
        if (ch == L',')  return VK_OEM_COMMA;
        if (ch == L'.')  return VK_OEM_PERIOD;
        if (ch == L'/')  return VK_OEM_2;
    }

    // Named keys (case-insensitive)
    if (_wcsicmp(name, L"BACKTICK")    == 0) return VK_OEM_3;
    if (_wcsicmp(name, L"TILDE")       == 0) return VK_OEM_3;
    if (_wcsicmp(name, L"TAB")         == 0) return VK_TAB;
    if (_wcsicmp(name, L"SPACE")       == 0) return VK_SPACE;
    if (_wcsicmp(name, L"ESCAPE")      == 0) return VK_ESCAPE;
    if (_wcsicmp(name, L"ESC")         == 0) return VK_ESCAPE;
    if (_wcsicmp(name, L"CAPSLOCK")    == 0) return VK_CAPITAL;
    if (_wcsicmp(name, L"INSERT")      == 0) return VK_INSERT;
    if (_wcsicmp(name, L"DELETE")      == 0) return VK_DELETE;
    if (_wcsicmp(name, L"HOME")        == 0) return VK_HOME;
    if (_wcsicmp(name, L"END")         == 0) return VK_END;
    if (_wcsicmp(name, L"PAGEUP")      == 0) return VK_PRIOR;
    if (_wcsicmp(name, L"PAGEDOWN")    == 0) return VK_NEXT;
    if (_wcsicmp(name, L"PAUSE")       == 0) return VK_PAUSE;
    if (_wcsicmp(name, L"SCROLLLOCK")  == 0) return VK_SCROLL;
    if (_wcsicmp(name, L"PRINTSCREEN") == 0) return VK_SNAPSHOT;

    // Function keys
    if (_wcsnicmp(name, L"F", 1) == 0 && name[1] >= L'0' && name[1] <= L'9') {
        int n = _wtoi(name + 1);
        if (n >= 1 && n <= 24) return VK_F1 + (n - 1);
    }

    // Numpad
    if (_wcsnicmp(name, L"NUMPAD", 6) == 0) {
        int n = _wtoi(name + 6);
        if (n >= 0 && n <= 9) return VK_NUMPAD0 + n;
    }

    return 0;
}

// =============================================================================
// Helper: Parse modifier flags from string
// =============================================================================
static UINT ModFromName(const WCHAR* name) {
    if (!name || !name[0] || _wcsicmp(name, L"NONE") == 0) return 0;

    UINT mod = 0;
    // Support combinations like "CTRL+ALT" or "CTRL+SHIFT"
    WCHAR buf[256];
    wcsncpy_s(buf, name, _TRUNCATE);

    WCHAR* ctx = NULL;
    WCHAR* tok = wcstok_s(buf, L"+| ", &ctx);
    while (tok) {
        if (_wcsicmp(tok, L"CTRL")  == 0 || _wcsicmp(tok, L"CONTROL") == 0)
            mod |= MOD_CONTROL;
        else if (_wcsicmp(tok, L"ALT") == 0)
            mod |= MOD_ALT;
        else if (_wcsicmp(tok, L"SHIFT") == 0)
            mod |= MOD_SHIFT;
        else if (_wcsicmp(tok, L"WIN") == 0)
            mod |= MOD_WIN;

        tok = wcstok_s(NULL, L"+| ", &ctx);
    }
    return mod;
}

// =============================================================================
// Load configuration from INI file
// =============================================================================
static void LoadConfig() {
    // Build INI path next to the executable
    GetModuleFileNameW(NULL, g_iniPath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(g_iniPath, L'\\');
    if (lastSlash) {
        wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash - g_iniPath + 1),
            L"WinQuickSwitch.ini");
    }

    // Read hotkey key name
    WCHAR keyName[64] = {};
    GetPrivateProfileStringW(L"Hotkey", L"Key", L"`",
        keyName, 64, g_iniPath);

    UINT vk = VkFromName(keyName);
    if (vk != 0) g_hotkeyVk = vk;

    // Read hotkey modifier
    WCHAR modName[64] = {};
    GetPrivateProfileStringW(L"Hotkey", L"Modifier", L"NONE",
        modName, 64, g_iniPath);
    g_hotkeyMod = ModFromName(modName);

    // Read start with windows preference
    g_startWithWindows = GetPrivateProfileIntW(L"General", L"StartWithWindows",
        0, g_iniPath);
}

// =============================================================================
// Create a simple icon programmatically (no .ico file needed)
// Draws a small "desktop switch" indicator
// =============================================================================
static HICON CreateDesktopSwitchIcon(int size) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    // Create the color bitmap
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = NULL;
    HBITMAP hbmColor = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS,
        &bits, NULL, 0);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmColor);

    // Draw background (transparent)
    DWORD* pixels = (DWORD*)bits;
    for (int i = 0; i < size * size; i++) {
        pixels[i] = 0x00000000;
    }

    // Draw two rectangles representing virtual desktops
    int pad = size / 8;
    int gap = size / 10;
    int rectW = (size - 2 * pad - gap) / 2;
    int rectH = size - 2 * pad;

    // Left rectangle (lighter blue - current desktop)
    DWORD colorActive = 0xFF4488CC;   // ARGB
    for (int y = pad; y < pad + rectH; y++) {
        for (int x = pad; x < pad + rectW; x++) {
            pixels[y * size + x] = colorActive;
        }
    }

    // Right rectangle (darker blue - other desktop)
    DWORD colorInactive = 0xFF225577;
    int rightX = pad + rectW + gap;
    for (int y = pad; y < pad + rectH; y++) {
        for (int x = rightX; x < rightX + rectW; x++) {
            pixels[y * size + x] = colorInactive;
        }
    }

    // Draw arrow between them
    DWORD colorArrow = 0xFFFFFFFF; // White
    int arrowY = size / 2;
    int arrowX = pad + rectW + gap / 2;
    // Horizontal line
    for (int dx = -gap / 2; dx <= gap / 2; dx++) {
        int px = arrowX + dx;
        if (px >= 0 && px < size) {
            pixels[arrowY * size + px] = colorArrow;
            if (arrowY > 0) pixels[(arrowY - 1) * size + px] = colorArrow;
        }
    }

    SelectObject(hdcMem, hbmOld);

    // Create mask bitmap (all opaque where we drew)
    HBITMAP hbmMask = CreateCompatibleBitmap(hdcScreen, size, size);
    HDC hdcMask = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmMaskOld = (HBITMAP)SelectObject(hdcMask, hbmMask);

    // Black = opaque, White = transparent
    RECT rc = {0, 0, size, size};
    HBRUSH hbrWhite = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HBRUSH hbrBlack = (HBRUSH)GetStockObject(BLACK_BRUSH);
    FillRect(hdcMask, &rc, hbrWhite);

    // Make drawn pixels opaque in mask
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if ((pixels[y * size + x] & 0xFF000000) != 0) {
                SetPixel(hdcMask, x, y, RGB(0, 0, 0));
            }
        }
    }

    SelectObject(hdcMask, hbmMaskOld);
    DeleteDC(hdcMask);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    // Create the icon
    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmMask = hbmMask;
    ii.hbmColor = hbmColor;

    HICON hIcon = CreateIconIndirect(&ii);

    DeleteObject(hbmColor);
    DeleteObject(hbmMask);

    return hIcon;
}

// =============================================================================
// System tray icon management
// =============================================================================
static void CreateTrayIcon(HWND hWnd) {
    ZeroMemory(&g_nid, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = CreateDesktopSwitchIcon(GetSystemMetrics(SM_CXSMICON));
    wcscpy_s(g_nid.szTip, APP_TOOLTIP);

    Shell_NotifyIconW(NIM_ADD, &g_nid);

    // Use version 4 features for better behavior
    g_nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &g_nid);
}

static void RemoveTrayIcon() {
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_nid.hIcon) {
        DestroyIcon(g_nid.hIcon);
        g_nid.hIcon = NULL;
    }
}

// =============================================================================
// Tray context menu
// =============================================================================
static void ShowTrayMenu(HWND hWnd) {
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    // Title item (disabled, bold)
    MENUITEMINFOW mii = {};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING | MIIM_STATE | MIIM_ID;
    mii.fState = MFS_DISABLED;
    mii.wID = ID_TRAY_FIRST;
    WCHAR title[128];
    _snwprintf_s(title, _TRUNCATE, L"WinQuickSwitch v%s", APP_VERSION);
    mii.dwTypeData = title;
    InsertMenuItemW(hMenu, 0, TRUE, &mii);

    // Show COM status
    WCHAR status[128];
    if (g_vdm.IsComMode()) {
        const WCHAR* verName = L"Unknown";
        switch (g_vdm.GetVersion()) {
            case WinVersion::Win10:      verName = L"Win10"; break;
            case WinVersion::Win11_21H2: verName = L"Win11 21H2"; break;
            case WinVersion::Win11_22H2: verName = L"Win11 22H2"; break;
            case WinVersion::Win11_23H2: verName = L"Win11 23H2"; break;
            case WinVersion::Win11_24H2: verName = L"Win11 24H2"; break;
            default: break;
        }
        _snwprintf_s(status, _TRUNCATE, L"Mode: COM (%s)", verName);
    } else {
        _snwprintf_s(status, _TRUNCATE, L"Mode: Keyboard (fallback)");
    }
    mii.wID = ID_TRAY_FIRST + 100;
    mii.dwTypeData = status;
    InsertMenuItemW(hMenu, 1, TRUE, &mii);

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // Start with Windows toggle
    BOOL autoStart = IsStartWithWindowsEnabled();
    AppendMenuW(hMenu, MF_STRING | (autoStart ? MF_CHECKED : 0),
        ID_TRAY_STARTUP, L"Start with Windows");

    // Reinitialize COM
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_REINIT, L"Reinitialize COM");

    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");

    // Show the menu at cursor position
    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    PostMessage(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

// =============================================================================
// Auto-start with Windows (registry)
// =============================================================================
static BOOL IsStartWithWindowsEnabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_READ, &hKey)
        != ERROR_SUCCESS)
        return FALSE;

    WCHAR value[MAX_PATH] = {};
    DWORD size = sizeof(value);
    BOOL exists = (RegQueryValueExW(hKey, REG_APP_NAME, NULL, NULL,
        (LPBYTE)value, &size) == ERROR_SUCCESS);
    RegCloseKey(hKey);
    return exists;
}

static void ToggleStartWithWindows() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0,
        KEY_SET_VALUE | KEY_READ, &hKey) != ERROR_SUCCESS)
        return;

    if (IsStartWithWindowsEnabled()) {
        // Remove auto-start
        RegDeleteValueW(hKey, REG_APP_NAME);
    } else {
        // Add auto-start
        WCHAR exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        RegSetValueExW(hKey, REG_APP_NAME, 0, REG_SZ,
            (LPBYTE)exePath, (DWORD)(wcslen(exePath) + 1) * sizeof(WCHAR));
    }
    RegCloseKey(hKey);
}

// =============================================================================
// Window Procedure
// =============================================================================
static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam,
    LPARAM lParam)
{
    switch (msg) {
        case WM_CREATE:
            CreateTrayIcon(hWnd);
            // Register the global hotkey
            if (!RegisterHotKey(hWnd, ID_HOTKEY_TOGGLE, g_hotkeyMod,
                g_hotkeyVk))
            {
                MessageBoxW(hWnd,
                    L"Failed to register hotkey.\n\n"
                    L"Another application may be using the same key.\n"
                    L"Edit WinQuickSwitch.ini to change the hotkey.",
                    APP_NAME, MB_OK | MB_ICONWARNING);
            }
            return 0;

        case WM_HOTKEY:
            if (wParam == ID_HOTKEY_TOGGLE) {
                g_vdm.Toggle();
            }
            return 0;

        case WM_TRAYICON:
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                    ShowTrayMenu(hWnd);
                    break;
                case WM_LBUTTONDBLCLK:
                    // Double-click tray icon: show about
                    {
                        WCHAR msg[512];
                        _snwprintf_s(msg, _TRUNCATE,
                            L"WinQuickSwitch v%s\n\n"
                            L"Quick virtual desktop toggle.\n"
                            L"No dependencies. No animation.\n\n"
                            L"Mode: %s\n"
                            L"Hotkey: %s\n\n"
                            L"Edit WinQuickSwitch.ini to configure.",
                            APP_VERSION,
                            g_vdm.IsComMode() ? L"COM (instant)" :
                                L"Keyboard (with animation)",
                            g_hotkeyMod == 0 ? L"Backtick (`)" : L"Custom");
                        MessageBoxW(hWnd, msg, APP_NAME,
                            MB_OK | MB_ICONINFORMATION);
                    }
                    break;
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_STARTUP:
                    ToggleStartWithWindows();
                    break;
                case ID_TRAY_REINIT:
                    g_vdm.Shutdown();
                    if (g_vdm.Initialize()) {
                        WCHAR msg[128];
                        _snwprintf_s(msg, _TRUNCATE,
                            L"Reinitialized. Mode: %s",
                            g_vdm.IsComMode() ? L"COM" : L"Keyboard");
                        MessageBoxW(hWnd, msg, APP_NAME,
                            MB_OK | MB_ICONINFORMATION);
                    }
                    break;
                case ID_TRAY_EXIT:
                    PostMessage(hWnd, WM_CLOSE, 0, 0);
                    break;
            }
            return 0;

        case WM_DESTROY:
            UnregisterHotKey(hWnd, ID_HOTKEY_TOGGLE);
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;

        // Handle taskbar recreation (e.g., explorer.exe restart)
        default:
            if (msg == RegisterWindowMessageW(L"TaskbarCreated")) {
                CreateTrayIcon(hWnd);
                return 0;
            }
            break;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// =============================================================================
// Ensure single instance using a named mutex
// =============================================================================
static bool AcquireSingleInstance() {
    HANDLE hMutex = CreateMutexW(NULL, TRUE,
        L"Global\\WinQuickSwitch_SingleInstance");
    if (hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        if (hMutex) CloseHandle(hMutex);
        return false;
    }
    // Intentionally leak the mutex handle - it lives for the process lifetime
    return true;
}

// =============================================================================
// WinMain - Application Entry Point
// =============================================================================
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Ensure single instance
    if (!AcquireSingleInstance()) {
        MessageBoxW(NULL,
            L"WinQuickSwitch is already running.\n"
            L"Check the system tray.",
            APP_NAME, MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    g_hInstance = hInstance;

    // Initialize COM (needed for virtual desktop access)
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(NULL, L"Failed to initialize COM.", APP_NAME,
            MB_OK | MB_ICONERROR);
        return 1;
    }

    // Load configuration
    LoadConfig();

    // Initialize virtual desktop manager
    if (!g_vdm.Initialize()) {
        // Non-fatal: will use keyboard fallback
    }

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"WinQuickSwitchClass";
    RegisterClassExW(&wc);

    // Create hidden message-only window
    g_hWnd = CreateWindowExW(0, L"WinQuickSwitchClass", APP_NAME,
        0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!g_hWnd) {
        CoUninitialize();
        return 1;
    }

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Cleanup
    g_vdm.Shutdown();
    CoUninitialize();

    return (int)msg.wParam;
}
