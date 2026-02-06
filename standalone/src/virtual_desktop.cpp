// =============================================================================
// virtual_desktop.cpp - Virtual Desktop Manager Implementation
//
// Implements direct COM-based virtual desktop switching for Windows 10/11.
// Falls back to keyboard simulation (Win+Ctrl+Arrow) if COM init fails.
// =============================================================================

#include "virtual_desktop.h"
#include <stdio.h>

// Helper: Get Windows build number from registry
static DWORD GetWindowsBuildNumber() {
    DWORD build = 0;
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
        0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        // Try CurrentBuildNumber (string)
        WCHAR buf[32] = {};
        DWORD size = sizeof(buf);
        if (RegQueryValueExW(hKey, L"CurrentBuildNumber", NULL, NULL,
            (LPBYTE)buf, &size) == ERROR_SUCCESS)
        {
            build = (DWORD)_wtoi(buf);
        }
        RegCloseKey(hKey);
    }
    return build;
}

// =============================================================================
// VirtualDesktopManager Implementation
// =============================================================================

VirtualDesktopManager::VirtualDesktopManager()
    : m_version(WinVersion::Unknown)
    , m_comInitialized(false)
    , m_toggleState(1)
    , m_pServiceProvider(nullptr)
    , m_pManagerInternal(nullptr)
{
}

VirtualDesktopManager::~VirtualDesktopManager() {
    Shutdown();
}

WinVersion VirtualDesktopManager::DetectWindowsVersion() {
    DWORD build = GetWindowsBuildNumber();

    if (build >= 26100) return WinVersion::Win11_24H2;
    if (build >= 22631) return WinVersion::Win11_23H2;
    if (build >= 22621) return WinVersion::Win11_22H2;
    if (build >= 22000) return WinVersion::Win11_21H2;
    if (build >= 19041) return WinVersion::Win10;

    return WinVersion::Unknown;
}

bool VirtualDesktopManager::InitializeCom() {
    HRESULT hr;

    // Create the ImmersiveShell instance
    IUnknown* pShell = nullptr;
    hr = CoCreateInstance(CLSID_ImmersiveShell, NULL, CLSCTX_LOCAL_SERVER,
        IID_IUnknown, (void**)&pShell);
    if (FAILED(hr) || !pShell) return false;

    // Query for IServiceProvider
    hr = pShell->QueryInterface(IID_IServiceProvider_,
        (void**)&m_pServiceProvider);
    pShell->Release();
    if (FAILED(hr) || !m_pServiceProvider) return false;

    // Select the right IID based on Windows version
    const IID* pIID = nullptr;
    switch (m_version) {
        case WinVersion::Win10:
            pIID = &IID_IVDMInternal_Win10;
            break;
        case WinVersion::Win11_21H2:
            pIID = &IID_IVDMInternal_Win11_21H2;
            break;
        case WinVersion::Win11_22H2:
            pIID = &IID_IVDMInternal_Win11_22H2;
            break;
        case WinVersion::Win11_23H2:
            pIID = &IID_IVDMInternal_Win11_23H2;
            break;
        case WinVersion::Win11_24H2:
            pIID = &IID_IVDMInternal_Win11_24H2;
            break;
        default:
            return false;
    }

    // Get the VirtualDesktopManagerInternal via service provider
    hr = m_pServiceProvider->QueryService(
        CLSID_VirtualDesktopManagerInternal,
        *pIID,
        (void**)&m_pManagerInternal);
    if (FAILED(hr) || !m_pManagerInternal) {
        m_pServiceProvider->Release();
        m_pServiceProvider = nullptr;
        return false;
    }

    // Validate: try to get desktop count as a sanity check
    int count = GetDesktopCount();
    if (count < 1 || count > 100) {
        // Something is wrong - interface mismatch
        m_pManagerInternal->Release();
        m_pManagerInternal = nullptr;
        m_pServiceProvider->Release();
        m_pServiceProvider = nullptr;
        return false;
    }

    return true;
}

bool VirtualDesktopManager::Initialize() {
    m_version = DetectWindowsVersion();

    if (m_version == WinVersion::Unknown) {
        // Unsupported Windows version, use keyboard fallback
        m_comInitialized = false;
        return true; // Still usable via fallback
    }

    // Try COM initialization
    m_comInitialized = InitializeCom();

    // If COM failed, try other version IIDs as fallback
    if (!m_comInitialized) {
        // Try all known IIDs in case detection was wrong
        const IID* fallbackIIDs[] = {
            &IID_IVDMInternal_Win11_24H2,
            &IID_IVDMInternal_Win11_23H2,
            &IID_IVDMInternal_Win11_22H2,
            &IID_IVDMInternal_Win11_21H2,
            &IID_IVDMInternal_Win10,
        };

        for (const IID* iid : fallbackIIDs) {
            // Re-create service provider if needed
            if (!m_pServiceProvider) {
                IUnknown* pShell = nullptr;
                HRESULT hr = CoCreateInstance(CLSID_ImmersiveShell, NULL,
                    CLSCTX_LOCAL_SERVER, IID_IUnknown, (void**)&pShell);
                if (FAILED(hr) || !pShell) break;

                hr = pShell->QueryInterface(IID_IServiceProvider_,
                    (void**)&m_pServiceProvider);
                pShell->Release();
                if (FAILED(hr)) break;
            }

            HRESULT hr = m_pServiceProvider->QueryService(
                CLSID_VirtualDesktopManagerInternal,
                *iid,
                (void**)&m_pManagerInternal);

            if (SUCCEEDED(hr) && m_pManagerInternal) {
                // Update version based on which IID worked
                if (iid == &IID_IVDMInternal_Win10)
                    m_version = WinVersion::Win10;
                else if (iid == &IID_IVDMInternal_Win11_21H2)
                    m_version = WinVersion::Win11_21H2;
                else if (iid == &IID_IVDMInternal_Win11_22H2)
                    m_version = WinVersion::Win11_22H2;
                else if (iid == &IID_IVDMInternal_Win11_23H2)
                    m_version = WinVersion::Win11_23H2;
                else if (iid == &IID_IVDMInternal_Win11_24H2)
                    m_version = WinVersion::Win11_24H2;

                // Validate with GetDesktopCount
                int count = GetDesktopCount();
                if (count >= 1 && count <= 100) {
                    m_comInitialized = true;
                    break;
                }

                // Bad result, release and try next
                m_pManagerInternal->Release();
                m_pManagerInternal = nullptr;
            }
        }
    }

    return true; // Always returns true - falls back to keyboard mode
}

void VirtualDesktopManager::Shutdown() {
    if (m_pManagerInternal) {
        m_pManagerInternal->Release();
        m_pManagerInternal = nullptr;
    }
    if (m_pServiceProvider) {
        m_pServiceProvider->Release();
        m_pServiceProvider = nullptr;
    }
    m_comInitialized = false;
}

// =============================================================================
// Desktop Count
// =============================================================================
int VirtualDesktopManager::GetDesktopCount() {
    if (!m_comInitialized || !m_pManagerInternal) return -1;

    UINT count = 0;
    HRESULT hr;

    switch (m_version) {
        case WinVersion::Win10: {
            auto* mgr = static_cast<IVDMInternal_Win10*>(m_pManagerInternal);
            hr = mgr->GetCount(&count);
            break;
        }
        case WinVersion::Win11_21H2:
        case WinVersion::Win11_22H2:
        case WinVersion::Win11_23H2: {
            auto* mgr = static_cast<IVDMInternal_Win11*>(m_pManagerInternal);
            hr = mgr->GetCount(&count);
            break;
        }
        case WinVersion::Win11_24H2: {
            auto* mgr = static_cast<IVDMInternal_Win11_24H2*>(m_pManagerInternal);
            hr = mgr->GetCount(NULL, &count);
            break;
        }
        default:
            return -1;
    }

    return SUCCEEDED(hr) ? (int)count : -1;
}

// =============================================================================
// Get Current Desktop Number (0-based)
// =============================================================================
int VirtualDesktopManager::GetCurrentDesktopNumber() {
    if (!m_comInitialized || !m_pManagerInternal) return -1;

    IUnknown* pCurrentDesktop = nullptr;
    IObjectArray10* pDesktops = nullptr;
    HRESULT hr;

    // Get current desktop object
    switch (m_version) {
        case WinVersion::Win10: {
            auto* mgr = static_cast<IVDMInternal_Win10*>(m_pManagerInternal);
            hr = mgr->GetCurrentDesktop(&pCurrentDesktop);
            break;
        }
        case WinVersion::Win11_21H2:
        case WinVersion::Win11_22H2:
        case WinVersion::Win11_23H2: {
            auto* mgr = static_cast<IVDMInternal_Win11*>(m_pManagerInternal);
            hr = mgr->GetCurrentDesktop(&pCurrentDesktop);
            break;
        }
        case WinVersion::Win11_24H2: {
            auto* mgr = static_cast<IVDMInternal_Win11_24H2*>(m_pManagerInternal);
            hr = mgr->GetCurrentDesktop(NULL, &pCurrentDesktop);
            break;
        }
        default:
            return -1;
    }

    if (FAILED(hr) || !pCurrentDesktop) return -1;

    // Get current desktop's GUID
    GUID currentId = {};
    bool isWin10 = (m_version == WinVersion::Win10);

    if (isWin10) {
        auto* desktop = static_cast<IVirtualDesktop_Win10*>(pCurrentDesktop);
        hr = desktop->GetID(&currentId);
    } else {
        auto* desktop = static_cast<IVirtualDesktop_Win11*>(pCurrentDesktop);
        hr = desktop->GetID(&currentId);
    }
    pCurrentDesktop->Release();

    if (FAILED(hr)) return -1;

    // Get all desktops
    switch (m_version) {
        case WinVersion::Win10: {
            auto* mgr = static_cast<IVDMInternal_Win10*>(m_pManagerInternal);
            hr = mgr->GetDesktops(&pDesktops);
            break;
        }
        case WinVersion::Win11_21H2:
        case WinVersion::Win11_22H2:
        case WinVersion::Win11_23H2: {
            auto* mgr = static_cast<IVDMInternal_Win11*>(m_pManagerInternal);
            hr = mgr->GetAllCurrentDesktops(&pDesktops);
            break;
        }
        case WinVersion::Win11_24H2: {
            auto* mgr = static_cast<IVDMInternal_Win11_24H2*>(m_pManagerInternal);
            hr = mgr->GetAllCurrentDesktops(&pDesktops);
            break;
        }
        default:
            return -1;
    }

    if (FAILED(hr) || !pDesktops) return -1;

    // Find the index of the current desktop
    UINT count = 0;
    pDesktops->GetCount(&count);

    const IID& iidDesktop = isWin10 ?
        IID_IVirtualDesktop_Win10 : IID_IVirtualDesktop_Win11;

    int result = -1;
    for (UINT i = 0; i < count; i++) {
        IUnknown* pDesktop = nullptr;
        hr = pDesktops->GetAt(i, iidDesktop, (void**)&pDesktop);
        if (SUCCEEDED(hr) && pDesktop) {
            GUID id = {};
            if (isWin10) {
                static_cast<IVirtualDesktop_Win10*>(pDesktop)->GetID(&id);
            } else {
                static_cast<IVirtualDesktop_Win11*>(pDesktop)->GetID(&id);
            }
            pDesktop->Release();

            if (IsEqualGUID(id, currentId)) {
                result = (int)i;
                break;
            }
        }
    }

    pDesktops->Release();
    return result;
}

// =============================================================================
// Switch to Desktop by Number (0-based)
// =============================================================================
bool VirtualDesktopManager::GoToDesktopNumber(int number) {
    if (!m_comInitialized || !m_pManagerInternal) {
        // Keyboard fallback
        int current = 0; // Can't know current in fallback mode
        if (number > current)
            SwitchDesktopRight();
        else
            SwitchDesktopLeft();
        return true;
    }

    if (number < 0) return false;

    IObjectArray10* pDesktops = nullptr;
    HRESULT hr;

    // Get all desktops
    switch (m_version) {
        case WinVersion::Win10: {
            auto* mgr = static_cast<IVDMInternal_Win10*>(m_pManagerInternal);
            hr = mgr->GetDesktops(&pDesktops);
            break;
        }
        case WinVersion::Win11_21H2:
        case WinVersion::Win11_22H2:
        case WinVersion::Win11_23H2: {
            auto* mgr = static_cast<IVDMInternal_Win11*>(m_pManagerInternal);
            hr = mgr->GetAllCurrentDesktops(&pDesktops);
            break;
        }
        case WinVersion::Win11_24H2: {
            auto* mgr = static_cast<IVDMInternal_Win11_24H2*>(m_pManagerInternal);
            hr = mgr->GetAllCurrentDesktops(&pDesktops);
            break;
        }
        default:
            return false;
    }

    if (FAILED(hr) || !pDesktops) return false;

    UINT count = 0;
    pDesktops->GetCount(&count);
    if ((UINT)number >= count) {
        pDesktops->Release();
        return false;
    }

    // Get the target desktop
    bool isWin10 = (m_version == WinVersion::Win10);
    const IID& iidDesktop = isWin10 ?
        IID_IVirtualDesktop_Win10 : IID_IVirtualDesktop_Win11;

    IUnknown* pTarget = nullptr;
    hr = pDesktops->GetAt((UINT)number, iidDesktop, (void**)&pTarget);
    pDesktops->Release();

    if (FAILED(hr) || !pTarget) return false;

    // Switch to the target desktop
    bool success = false;
    switch (m_version) {
        case WinVersion::Win10: {
            auto* mgr = static_cast<IVDMInternal_Win10*>(m_pManagerInternal);
            hr = mgr->SwitchDesktop(pTarget);
            success = SUCCEEDED(hr);
            break;
        }
        case WinVersion::Win11_21H2:
        case WinVersion::Win11_22H2:
        case WinVersion::Win11_23H2: {
            auto* mgr = static_cast<IVDMInternal_Win11*>(m_pManagerInternal);
            hr = mgr->SwitchDesktop(pTarget);
            success = SUCCEEDED(hr);
            break;
        }
        case WinVersion::Win11_24H2: {
            auto* mgr = static_cast<IVDMInternal_Win11_24H2*>(m_pManagerInternal);
            hr = mgr->SwitchDesktop(NULL, pTarget);
            success = SUCCEEDED(hr);
            break;
        }
        default:
            break;
    }

    pTarget->Release();
    return success;
}

// =============================================================================
// Toggle - Replicate the AHK script's toggle behavior
// =============================================================================
void VirtualDesktopManager::Toggle() {
    if (m_comInitialized) {
        int current = GetCurrentDesktopNumber();
        int count = GetDesktopCount();

        if (current < 0 || count < 0) {
            // COM call failed, try keyboard fallback
            if (m_toggleState) {
                SwitchDesktopLeft();
                m_toggleState = 0;
            } else {
                SwitchDesktopRight();
                m_toggleState = 1;
            }
            return;
        }

        if (m_toggleState == 1) {
            // Try to go left
            if (current > 0) {
                GoToDesktopNumber(current - 1);
            }
            m_toggleState = 0;
        } else {
            // Try to go right
            if (current < count - 1) {
                GoToDesktopNumber(current + 1);
            }
            m_toggleState = 1;
        }
    } else {
        // Pure keyboard fallback
        if (m_toggleState) {
            SwitchDesktopLeft();
            m_toggleState = 0;
        } else {
            SwitchDesktopRight();
            m_toggleState = 1;
        }
    }
}

// =============================================================================
// Keyboard Fallback: Simulate Win+Ctrl+Left/Right
// =============================================================================
void VirtualDesktopManager::SwitchDesktopLeft() {
    INPUT inputs[6] = {};

    // Key down: Win
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;

    // Key down: Ctrl
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_LCONTROL;

    // Key down: Left arrow
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_LEFT;

    // Key up: Left arrow
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_LEFT;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    // Key up: Ctrl
    inputs[4].type = INPUT_KEYBOARD;
    inputs[4].ki.wVk = VK_LCONTROL;
    inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;

    // Key up: Win
    inputs[5].type = INPUT_KEYBOARD;
    inputs[5].ki.wVk = VK_LWIN;
    inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(6, inputs, sizeof(INPUT));
}

void VirtualDesktopManager::SwitchDesktopRight() {
    INPUT inputs[6] = {};

    // Key down: Win
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_LWIN;

    // Key down: Ctrl
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_LCONTROL;

    // Key down: Right arrow
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = VK_RIGHT;

    // Key up: Right arrow
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_RIGHT;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    // Key up: Ctrl
    inputs[4].type = INPUT_KEYBOARD;
    inputs[4].ki.wVk = VK_LCONTROL;
    inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;

    // Key up: Win
    inputs[5].type = INPUT_KEYBOARD;
    inputs[5].ki.wVk = VK_LWIN;
    inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(6, inputs, sizeof(INPUT));
}
