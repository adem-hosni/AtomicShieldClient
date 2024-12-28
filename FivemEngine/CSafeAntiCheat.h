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

using ArgType = std::variant<int, DWORD64, std::string, bool>;

enum eDetectionType
{
    UNAUTHORIZED_THREAD = 1,
    UNRECOGNISED_IAT_FOUND,
    DLL_FOUND,
    SECURE_BOOT_DISABLED,
    DEBUG_MODE_ENABLED,
    TEST_SIGNING_ENABLED,
    INJECTED_DLL,
    CHEAT_SIGNATURE_FOUND,
    MALICIOUS_PROCESS_HANDLE_OPEN,
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

    void StartBasicChecks();

    void        CheckPlugins();
    void        DebugModeEnabled();
    void        SecureBootEnabled();
    void        TestsigningEnabled();
    std::string GetWindowsDrive();

    void NotifyDetection(eDetectionType DetectionType, std::unordered_map<std::string, ArgType> kwargs = {});

    HANDLE GetProcessHandle() { return m_hProcess; }
    int    GetProcessID() { return m_iTargetProcessID; }

    bool                         IsAtomicThread(HANDLE hThread);
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