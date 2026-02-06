#pragma once
// =============================================================================
// virtual_desktop.h - Windows Virtual Desktop COM Interface Definitions
//
// Provides direct access to Windows 10/11 virtual desktop switching without
// external dependencies. Uses undocumented COM interfaces reverse-engineered
// by the community (Ciantic/VirtualDesktopAccessor, FancyZones, etc.)
//
// Supported builds:
//   Windows 10 19041-19045 (20H1 through 22H2)
//   Windows 11 22000       (21H2)
//   Windows 11 22621       (22H2)
//   Windows 11 22631       (23H2)
//   Windows 11 26100+      (24H2)
// =============================================================================

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <objbase.h>
#include <unknwn.h>

// HSTRING is a WinRT type; we only need it as an opaque pointer for vtable padding
#ifndef __HSTRING_DEFINED
#define __HSTRING_DEFINED
typedef struct HSTRING__* HSTRING;
#endif

// =============================================================================
// GUIDs - Stable across Windows versions
// =============================================================================

// CLSID for the Immersive Shell broker
// {C2F03A33-21F5-47FA-B4BB-156362A2F239}
static const CLSID CLSID_ImmersiveShell = {
    0xC2F03A33, 0x21F5, 0x47FA,
    {0xB4, 0xBB, 0x15, 0x63, 0x62, 0xA2, 0xF2, 0x39}
};

// CLSID for VirtualDesktopManagerInternal service
// {C5E0CDCA-7B6E-41B2-9FC4-D93975CC467B}
static const CLSID CLSID_VirtualDesktopManagerInternal = {
    0xC5E0CDCA, 0x7B6E, 0x41B2,
    {0x9F, 0xC4, 0xD9, 0x39, 0x75, 0xCC, 0x46, 0x7B}
};

// IID for IServiceProvider (standard COM)
// {6D5140C1-7436-11CE-8034-00AA006009FA}
static const IID IID_IServiceProvider_ = {
    0x6D5140C1, 0x7436, 0x11CE,
    {0x80, 0x34, 0x00, 0xAA, 0x00, 0x60, 0x09, 0xFA}
};

// IID for IObjectArray (standard COM, from objectarray.h)
// {92CA9DCD-5622-4BBA-A805-5E9F541BD8C9}
static const IID IID_IObjectArray_ = {
    0x92CA9DCD, 0x5622, 0x4BBA,
    {0xA8, 0x05, 0x5E, 0x9F, 0x54, 0x1B, 0xD8, 0xC9}
};

// =============================================================================
// Version-specific IIDs for IVirtualDesktop
// =============================================================================

// Windows 10 (Build 19041-19045)
// {FF72FFDD-BE7E-43FC-9C03-AD81681E88E4}
static const IID IID_IVirtualDesktop_Win10 = {
    0xFF72FFDD, 0xBE7E, 0x43FC,
    {0x9C, 0x03, 0xAD, 0x81, 0x68, 0x1E, 0x88, 0xE4}
};

// Windows 11 (Build 22000+)
// {3F07F4BE-B107-441A-AF0F-39D82529072C}
static const IID IID_IVirtualDesktop_Win11 = {
    0x3F07F4BE, 0xB107, 0x441A,
    {0xAF, 0x0F, 0x39, 0xD8, 0x25, 0x29, 0x07, 0x2C}
};

// =============================================================================
// Version-specific IIDs for IVirtualDesktopManagerInternal
// =============================================================================

// Windows 10 Build 19041-19045
// {F31574D6-B682-4CDC-BD56-1827860ABEC6}
static const IID IID_IVDMInternal_Win10 = {
    0xF31574D6, 0xB682, 0x4CDC,
    {0xBD, 0x56, 0x18, 0x27, 0x86, 0x0A, 0xBE, 0xC6}
};

// Windows 11 Build 22000 (21H2)
// {B2F925B9-5A0F-4D2E-9F4D-2B1507593C10}
static const IID IID_IVDMInternal_Win11_21H2 = {
    0xB2F925B9, 0x5A0F, 0x4D2E,
    {0x9F, 0x4D, 0x2B, 0x15, 0x07, 0x59, 0x3C, 0x10}
};

// Windows 11 Build 22621 (22H2)
// {B2F925B9-5A0F-4D2E-9F4D-2B1507593C10}  (same as 21H2)
static const IID IID_IVDMInternal_Win11_22H2 = {
    0xB2F925B9, 0x5A0F, 0x4D2E,
    {0x9F, 0x4D, 0x2B, 0x15, 0x07, 0x59, 0x3C, 0x10}
};

// Windows 11 Build 22631 (23H2)
// {A3175F2D-239C-4BD2-8AA0-EEBA8B0B138E}
static const IID IID_IVDMInternal_Win11_23H2 = {
    0xA3175F2D, 0x239C, 0x4BD2,
    {0x8A, 0xA0, 0xEE, 0xBA, 0x8B, 0x0B, 0x13, 0x8E}
};

// Windows 11 Build 26100 (24H2)
// {53F5CA0B-158F-4124-900C-057158060B27}
static const IID IID_IVDMInternal_Win11_24H2 = {
    0x53F5CA0B, 0x158F, 0x4124,
    {0x90, 0x0C, 0x05, 0x71, 0x58, 0x06, 0x0B, 0x27}
};

// =============================================================================
// Minimal COM Interface Definitions
// We define only the methods we need, with padding for vtable alignment
// =============================================================================

// IServiceProvider - standard COM interface for service querying
MIDL_INTERFACE("6D5140C1-7436-11CE-8034-00AA006009FA")
IServiceProvider10 : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE QueryService(
        REFGUID guidService,
        REFIID riid,
        void** ppvObject) = 0;
};

// IObjectArray - standard COM interface for object collections
MIDL_INTERFACE("92CA9DCD-5622-4BBA-A805-5E9F541BD8C9")
IObjectArray10 : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetCount(UINT* pcObjects) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAt(
        UINT uiIndex,
        REFIID riid,
        void** ppv) = 0;
};

// =============================================================================
// IVirtualDesktop - Windows 10
// vtable: QueryInterface, AddRef, Release, IsViewVisible, GetID
// =============================================================================
MIDL_INTERFACE("FF72FFDD-BE7E-43FC-9C03-AD81681E88E4")
IVirtualDesktop_Win10 : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE IsViewVisible(
        IUnknown* pView, int* pfVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetID(GUID* pGuid) = 0;
};

// =============================================================================
// IVirtualDesktop - Windows 11
// vtable: QueryInterface, AddRef, Release, IsViewVisible, GetID, GetName, GetWallpaper
// =============================================================================
MIDL_INTERFACE("3F07F4BE-B107-441A-AF0F-39D82529072C")
IVirtualDesktop_Win11 : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE IsViewVisible(
        IUnknown* pView, int* pfVisible) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetID(GUID* pGuid) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetName(HSTRING* pName) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetWallpaper(HSTRING* pPath) = 0;
};

// =============================================================================
// IVirtualDesktopManagerInternal - Windows 10 (Build 19041-19045)
//
// vtable layout:
//  [3] GetCount(UINT*)
//  [4] MoveViewToDesktop(IApplicationView*, IVirtualDesktop*)
//  [5] CanViewMoveDesktops(IApplicationView*, BOOL*)
//  [6] GetCurrentDesktop(IVirtualDesktop**)
//  [7] GetDesktops(IObjectArray**)
//  [8] AdjacentDesktop(IVirtualDesktop*, int, IVirtualDesktop**)
//  [9] SwitchDesktop(IVirtualDesktop*)
// =============================================================================
MIDL_INTERFACE("F31574D6-B682-4CDC-BD56-1827860ABEC6")
IVDMInternal_Win10 : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetCount(UINT* pCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(
        IUnknown* pView, IUnknown* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(
        IUnknown* pView, BOOL* pfCanMove) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(
        IUnknown** ppDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDesktops(
        IObjectArray10** ppDesktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE AdjacentDesktop(
        IUnknown* pDesktopFrom, int nDirection,
        IUnknown** ppDesktopTo) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(
        IUnknown* pDesktop) = 0;
};

// =============================================================================
// IVirtualDesktopManagerInternal - Windows 11 22000-22631 (21H2, 22H2, 23H2)
//
// vtable layout:
//  [3] GetCount(UINT*)
//  [4] MoveViewToDesktop(IApplicationView*, IVirtualDesktop*)
//  [5] CanViewMoveDesktops(IApplicationView*, BOOL*)
//  [6] GetCurrentDesktop(IVirtualDesktop**)
//  [7] GetAllCurrentDesktops(IObjectArray**)
//  [8] AdjacentDesktop(...)
//  [9] SwitchDesktop(IVirtualDesktop*)
//  [10] SwitchDesktopAndMoveForegroundView(IVirtualDesktop*)
//  [11] CreateDesktop(IVirtualDesktop**)
//  [12] MoveDesktop(...)
//  [13] RemoveDesktop(IVirtualDesktop*, IVirtualDesktop*)
//  [14] FindDesktop(GUID*, IVirtualDesktop**)
// =============================================================================
MIDL_INTERFACE("B2F925B9-5A0F-4D2E-9F4D-2B1507593C10")
IVDMInternal_Win11 : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetCount(UINT* pCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(
        IUnknown* pView, IUnknown* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(
        IUnknown* pView, BOOL* pfCanMove) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(
        IUnknown** ppDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAllCurrentDesktops(
        IObjectArray10** ppDesktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE AdjacentDesktop(
        IUnknown* pDesktopFrom, int nDirection,
        IUnknown** ppDesktopTo) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(
        IUnknown* pDesktop) = 0;
};

// =============================================================================
// IVirtualDesktopManagerInternal - Windows 11 26100+ (24H2)
//
// Key difference: GetCount, GetCurrentDesktop, SwitchDesktop take HWND param
//
//  [3] GetCount(HWND, UINT*)
//  [4] MoveViewToDesktop(IApplicationView*, IVirtualDesktop*)
//  [5] CanViewMoveDesktops(IApplicationView*, BOOL*)
//  [6] GetCurrentDesktop(HWND, IVirtualDesktop**)
//  [7] GetAllCurrentDesktops(IObjectArray**)
//  [8] AdjacentDesktop(...)
//  [9] SwitchDesktop(HWND, IVirtualDesktop*)
// =============================================================================
MIDL_INTERFACE("53F5CA0B-158F-4124-900C-057158060B27")
IVDMInternal_Win11_24H2 : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetCount(HWND hwnd, UINT* pCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE MoveViewToDesktop(
        IUnknown* pView, IUnknown* pDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE CanViewMoveDesktops(
        IUnknown* pView, BOOL* pfCanMove) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentDesktop(
        HWND hwnd, IUnknown** ppDesktop) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetAllCurrentDesktops(
        IObjectArray10** ppDesktops) = 0;
    virtual HRESULT STDMETHODCALLTYPE AdjacentDesktop(
        IUnknown* pDesktopFrom, int nDirection,
        IUnknown** ppDesktopTo) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchDesktop(
        HWND hwnd, IUnknown* pDesktop) = 0;
};

// =============================================================================
// Windows version enum for internal use
// =============================================================================
enum class WinVersion {
    Unknown = 0,
    Win10,          // Build 19041-19045
    Win11_21H2,     // Build 22000
    Win11_22H2,     // Build 22621
    Win11_23H2,     // Build 22631
    Win11_24H2,     // Build 26100+
};

// =============================================================================
// VirtualDesktopManager - High-level wrapper for virtual desktop operations
// =============================================================================
class VirtualDesktopManager {
public:
    VirtualDesktopManager();
    ~VirtualDesktopManager();

    // Initialize COM interfaces. Returns true on success.
    bool Initialize();

    // Release COM interfaces
    void Shutdown();

    // Is COM mode active (false = keyboard fallback)
    bool IsComMode() const { return m_comInitialized; }

    // Get the detected Windows version
    WinVersion GetVersion() const { return m_version; }

    // Get total number of virtual desktops
    int GetDesktopCount();

    // Get the current desktop number (0-based)
    int GetCurrentDesktopNumber();

    // Switch to the specified desktop (0-based). Returns true on success.
    bool GoToDesktopNumber(int number);

    // Toggle between adjacent desktops (replicates AHK script behavior)
    void Toggle();

private:
    WinVersion DetectWindowsVersion();
    bool InitializeCom();
    int GetDesktopIndexFromObject(IUnknown* pDesktop);

    // Keyboard fallback methods
    void SwitchDesktopLeft();
    void SwitchDesktopRight();

    WinVersion m_version;
    bool m_comInitialized;
    int m_toggleState;  // 0 or 1, for toggling direction

    // COM pointers (untyped - cast based on version)
    IServiceProvider10* m_pServiceProvider;
    IUnknown* m_pManagerInternal;
};
