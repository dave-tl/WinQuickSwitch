@echo off
REM =============================================================================
REM build_mingw.bat - Build WinQuickSwitch with MinGW-w64 / g++
REM
REM Usage:
REM   1. Ensure g++ (MinGW-w64) is in your PATH
REM   2. Navigate to this directory
REM   3. Run: build_mingw.bat
REM
REM Output: WinQuickSwitch.exe in this directory
REM =============================================================================

setlocal enabledelayedexpansion

echo.
echo ============================================
echo  Building WinQuickSwitch (MinGW-w64)
echo ============================================
echo.

REM Check if g++ is available
where g++ >nul 2>&1
if errorlevel 1 (
    echo ERROR: g++ not found.
    echo.
    echo Please install MinGW-w64 and ensure g++ is in your PATH.
    echo Download: https://www.mingw-w64.org/
    echo.
    exit /b 1
)

REM Check if windres is available
where windres >nul 2>&1
if errorlevel 1 (
    echo WARNING: windres not found. Building without resources.
    echo.
    goto :no_resources
)

REM Compile resource file
echo [1/3] Compiling resources...
windres src\app.rc -o src\app.res.o
if errorlevel 1 (
    echo WARNING: Resource compilation failed. Building without resources.
    goto :no_resources
)

REM Compile with resources
echo [2/3] Compiling source...
g++ -O2 -Wall -Wextra -DUNICODE -D_UNICODE -DNDEBUG ^
    -Isrc ^
    -mwindows ^
    src\main.cpp src\virtual_desktop.cpp src\app.res.o ^
    -o WinQuickSwitch.exe ^
    -luser32 -lshell32 -lole32 -ladvapi32 -luuid -lcomctl32
if errorlevel 1 (
    echo ERROR: Compilation failed.
    exit /b 1
)

echo [3/3] Cleaning up...
del /q src\app.res.o 2>nul
goto :done

:no_resources
REM Compile without resources
echo [2/3] Compiling source (no resources)...
g++ -O2 -Wall -Wextra -DUNICODE -D_UNICODE -DNDEBUG ^
    -Isrc ^
    -mwindows ^
    src\main.cpp src\virtual_desktop.cpp ^
    -o WinQuickSwitch.exe ^
    -luser32 -lshell32 -lole32 -ladvapi32 -luuid -lcomctl32
if errorlevel 1 (
    echo ERROR: Compilation failed.
    exit /b 1
)

:done
echo.
echo ============================================
echo  Build successful: WinQuickSwitch.exe
echo ============================================
echo.
echo To run: WinQuickSwitch.exe
echo The app will appear in your system tray.
echo.

endlocal
