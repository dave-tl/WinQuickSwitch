@echo off
REM =============================================================================
REM build.bat - Build WinQuickSwitch with MSVC (Visual Studio)
REM
REM Usage:
REM   1. Open "Developer Command Prompt for Visual Studio"
REM   2. Navigate to this directory
REM   3. Run: build.bat
REM
REM   Or for a specific architecture:
REM   build.bat x64        (default)
REM   build.bat x86
REM
REM Output: WinQuickSwitch.exe in this directory
REM =============================================================================

setlocal enabledelayedexpansion

set ARCH=%1
if "%ARCH%"=="" set ARCH=x64

echo.
echo ============================================
echo  Building WinQuickSwitch (%ARCH%)
echo ============================================
echo.

REM Check if cl.exe is available
where cl >nul 2>&1
if errorlevel 1 (
    echo ERROR: cl.exe not found.
    echo.
    echo Please run this from a Visual Studio Developer Command Prompt,
    echo or run vcvarsall.bat first:
    echo.
    echo   "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" %ARCH%
    echo.
    exit /b 1
)

REM Compile resource file
echo [1/3] Compiling resources...
rc /nologo /fo src\app.res src\app.rc
if errorlevel 1 (
    echo ERROR: Resource compilation failed.
    exit /b 1
)

REM Compile source files
echo [2/3] Compiling source...
cl /nologo /W4 /O2 /EHsc /DUNICODE /D_UNICODE /DNDEBUG ^
    /Isrc ^
    src\main.cpp src\virtual_desktop.cpp ^
    src\app.res ^
    /Fe:WinQuickSwitch.exe ^
    /link /SUBSYSTEM:WINDOWS ^
    user32.lib shell32.lib ole32.lib advapi32.lib uuid.lib
if errorlevel 1 (
    echo ERROR: Compilation failed.
    exit /b 1
)

REM Clean up intermediate files
echo [3/3] Cleaning up...
del /q *.obj 2>nul
del /q src\app.res 2>nul

echo.
echo ============================================
echo  Build successful: WinQuickSwitch.exe
echo ============================================
echo.
echo To run: WinQuickSwitch.exe
echo The app will appear in your system tray.
echo.

endlocal
