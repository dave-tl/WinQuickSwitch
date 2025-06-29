# 🖥️ Simple AHK Script for Switching Virtual Desktops (No Animation)

**Fast virtual desktop switching without transition animations**

**Configurable single toggle key that triggers virtual desktop switching**

## 📋 Requirements
- **script** 📥[Download here](https://github.com/dave-tl/WinQuickSwitch/blob/main/quickswitch.ahk)
- **AutoHotkey v2+**
  📥[Download here](https://www.autohotkey.com/download/)
- **VirtualDesktopAccessor DLL** (kills the animations)  
  📥 [Download here](https://github.com/Ciantic/VirtualDesktopAccessor)

## 📁 Setup

**Important:** VirtualDesktopAccessor.dll and this AHK script should be in the same folder (preferably in the Startup folder, which can be found easily through `Win+R: shell:startup`).

## ⚙️ Customization

The toggle key can easily be remapped in `quickswitch.ahk` on the 4th line of the script:
```ahk
TOGGLE_KEY := "~"
```
