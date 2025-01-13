#pragma once
#include "StdInc.h"
#include "CAtomicNetwork.h"
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
    BLACKLISTED_DRIVER_LOADED
};

class CAtomicAntiCheat
{
public:
    CAtomicAntiCheat();
    ~CAtomicAntiCheat();

    bool Initialize();

    CAtomicNetwork* GetNetwork() { return m_pAtomicNetwork; }
    STiming&        GetTiming() { return m_Timing; }
    CGuardManager*  GetGuardManager() { return m_pGuardManager; }
    jsoncons::json  GetCurrentHWIDCache() { return m_HWIDCache; }

    static void StaticPulse(void* pContext);
    void        DoPulse();
    void        StartPulse();

    void StartBasicChecks();

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

    CAtomicNetwork* m_pAtomicNetwork;
    CGuardManager*  m_pGuardManager;

    std::vector<eDetectionType> m_vDetectedTypes;
    std::vector<CAtomicThread*> m_vAtomicThreads;
};

extern CAtomicAntiCheat* g_pAtomicAntiCheat;