# WinQuickSwitch - Standalone Edition

A single-executable Windows application for instant virtual desktop switching. No dependencies, no installation, no runtime requirements.

## What It Does

Press a hotkey (backtick `` ` `` by default) to toggle between adjacent virtual desktops **without transition animations**. This is significantly faster than the native `Win+Ctrl+Arrow` shortcut.

## Features

- **Zero dependencies** - Single `.exe` file, no installers, no DLLs, no runtimes
- **No animation** - Switches desktops instantly via direct COM API calls
- **Configurable hotkey** - Change the toggle key via `WinQuickSwitch.ini`
- **System tray** - Runs quietly in the background with a right-click menu
- **Start with Windows** - Optional auto-launch on login
- **Broad compatibility** - Supports Windows 10 (20H1+) and Windows 11 (all versions through 24H2)
- **Automatic fallback** - Falls back to keyboard simulation if COM APIs are unavailable

## Quick Start (Pre-built)

1. Download `WinQuickSwitch.exe` from Releases
2. Optionally place `WinQuickSwitch.ini` next to it to customize the hotkey
3. Run `WinQuickSwitch.exe`
4. The app appears in your system tray
5. Press `` ` `` (backtick) to toggle between desktops

## Building From Source

### Option A: Visual Studio (MSVC)

1. Open **Developer Command Prompt for Visual Studio**
2. Navigate to the `standalone` directory
3. Run:
   ```
   build.bat
   ```

### Option B: MinGW-w64

1. Ensure `g++` and `windres` are in your PATH
2. Navigate to the `standalone` directory
3. Run:
   ```
   build_mingw.bat
   ```

### Option C: Manual compilation

```
cl /W4 /O2 /EHsc /DUNICODE /D_UNICODE /Isrc src\main.cpp src\virtual_desktop.cpp /Fe:WinQuickSwitch.exe /link /SUBSYSTEM:WINDOWS user32.lib shell32.lib ole32.lib advapi32.lib uuid.lib
```

## Configuration

Edit `WinQuickSwitch.ini` (place it next to the `.exe`):

```ini
[Hotkey]
; The toggle key. Default: ` (backtick)
Key=`

; Optional modifier: NONE, CTRL, ALT, SHIFT, WIN (combine with +)
Modifier=NONE

[General]
; Auto-start with Windows (0 or 1)
StartWithWindows=0
```

### Hotkey Examples

| Config | Effect |
|--------|--------|
| `Key=`` ` | Backtick alone (default) |
| `Key=F12` | F12 key |
| `Key=D` + `Modifier=CTRL+ALT` | Ctrl+Alt+D |
| `Key=TAB` + `Modifier=WIN` | Win+Tab (not recommended) |
| `Key=SPACE` + `Modifier=CTRL` | Ctrl+Space |

### Supported Key Names

Single characters: `` ` `` `-` `=` `[` `]` `\` `;` `'` `,` `.` `/` `A-Z` `0-9`

Named keys: `BACKTICK` `TAB` `SPACE` `ESCAPE` `CAPSLOCK` `INSERT` `DELETE` `HOME` `END` `PAGEUP` `PAGEDOWN` `PAUSE` `SCROLLLOCK` `PRINTSCREEN` `F1`-`F24` `NUMPAD0`-`NUMPAD9`

## System Tray Menu

Right-click the tray icon to access:

- **Status display** - Shows active mode (COM or Keyboard fallback) and detected Windows version
- **Start with Windows** - Toggle auto-launch on login
- **Reinitialize COM** - Re-detect Windows version and reinitialize COM interfaces
- **Exit** - Close the application

Double-click the tray icon for an About dialog.

## How It Works

The application uses Windows' internal COM interfaces (`IVirtualDesktopManagerInternal`) to switch desktops directly, bypassing the animation system. This is the same technique used by tools like [VirtualDesktopAccessor](https://github.com/Ciantic/VirtualDesktopAccessor), but compiled directly into the executable.

At startup, the app:
1. Detects the Windows build number
2. Selects the appropriate COM interface GUIDs for that build
3. Initializes the virtual desktop manager via COM
4. Falls back to keyboard simulation (`Win+Ctrl+Arrow`) if COM fails

## Compatibility

| Windows Version | Build | Status |
|----------------|-------|--------|
| Windows 10 20H1-22H2 | 19041-19045 | Supported (COM) |
| Windows 11 21H2 | 22000 | Supported (COM) |
| Windows 11 22H2 | 22621 | Supported (COM) |
| Windows 11 23H2 | 22631 | Supported (COM) |
| Windows 11 24H2 | 26100+ | Supported (COM) |
| Other versions | - | Keyboard fallback |

## Troubleshooting

**"Failed to register hotkey"** - Another application is using the same hotkey. Change the key in `WinQuickSwitch.ini`.

**Switching has animation** - The app fell back to keyboard mode. Right-click the tray icon to check the status. Try "Reinitialize COM" or restart the app.

**App doesn't appear** - Check if it's already running (system tray). Only one instance can run at a time.

## Comparison with AHK Version

| Feature | AHK Script | Standalone |
|---------|-----------|------------|
| Dependencies | AutoHotkey v2 + DLL | None |
| File size | ~2 KB script + ~100 KB DLL | Single ~50 KB .exe |
| Configuration | Edit script source | INI file |
| System tray | No | Yes |
| Auto-start | Manual shortcut setup | Built-in toggle |
| Win version support | Via DLL updates | Built-in multi-version |
