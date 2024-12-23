#pragma once
#include "StdInc.h"
#include "CSafeNetwork.h"
#include "STiming.h"
#include "CGuardManager.h"

struct SMemoryDetectionReport
{
    LPVOID  BaseAddress;
    DWORD64 AllocatedProtect;
    PVOID   AllocatedBase;
    DWORD   RegionSize;
};

enum eDetectionType
{
    UNAUTHORIZED_THREAD = 1,
    UNRECOGNISED_IAT_FOUND = 2,
    DLL_FOUND = 3,
    SECURE_BOOT_DISABLED = 4,
    DEBUG_MODE_ENABLED = 5,
    TEST_SIGNING_ENABLED = 6
};

class CSafeAntiCheat
{
public:
    CSafeAntiCheat();
    ~CSafeAntiCheat();

    bool Initialize();

    CSafeNetwork*  GetNetwork() { return m_pSafeNetwork; }
    STiming&       GetTiming() { return m_Timing; }
    CGuardManager* GetGuardManager() { return m_pGuardManager; }
    jsoncons::json GetCurrentHWIDCache() { return m_HWIDCache; }

    static void StaticPulse(void* pContext);
    void        DoPulse();
    void        StartPulse();

    void        CheckPlugins();
    void        DebugModeEnabled();
    void        SecureBootEnabled();
    void        TestsigningEnabled();
    std::string GetWindowsDrive();

    void        NotifyDetection(eDetectionType DetectionType, SMemoryDetectionReport* pDetectionInfo);

    HANDLE GetProcessHandle() { return m_hProcess; }
    int    GetProcessID() { return m_iTargetProcessID; }

    bool IsAtomicThread(HANDLE hThread);
    std::vector<CAtomicThread*>& GetAtomicThreads() { return m_vAtomicThreads; }

private:
    int     m_iTargetProcessID;
    HANDLE  m_hProcess;
    STiming m_Timing;

    jsoncons::json m_HWIDCache;

    CSafeNetwork*  m_pSafeNetwork;
    CGuardManager* m_pGuardManager;

    std::vector<eDetectionType> m_vDetectedTypes;
    std::vector<CAtomicThread*> m_vAtomicThreads;
};

extern CSafeAntiCheat* g_pSafeAntiCheat;