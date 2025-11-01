#pragma once
#include "StdInc.h"
#include "CAtomicNetwork.h"
#include "CGuardManager.h"
#include "KernelCalls.hpp"

struct SMemoryDetectionReport
{
    LPVOID  BaseAddress;
    DWORD64 AllocatedProtect;
    PVOID   AllocatedBase;
    DWORD   RegionSize;
};

enum class eHardKickReason
{
    GUARD_TIMEDOUT = 1,
    PROCESS_TERMINATED,
    DEBUGGER_DETECTED,
};


using ArgType = std::variant<int, DWORD64, std::string, std::wstring, bool>;

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
    BLACKLISTED_DRIVER_LOADED,
    THREAD_SHELLCODE,
    MEMORY_INTEGRITY_DISABLED
};

class CAtomicAntiCheat
{
public:
    CAtomicAntiCheat();
    ~CAtomicAntiCheat();

    void SetAntiCheatModuleBase(LPVOID lpModuleBase) { m_lpAntiCheatModuleBase = lpModuleBase; };
    LPVOID GetAntiCheatModuleBase() { return m_lpAntiCheatModuleBase; }

    bool Initialize();

    CAtomicNetwork* GetNetwork() { return m_pAtomicNetwork; }
    CGuardManager*  GetGuardManager() { return m_pGuardManager; }
    jsoncons::json  GetCurrentHWIDCache() { return m_HWIDCache; }

    static void StaticPulse(void* pContext);
    void        DoPulse();
    void        StartPulse();

    CAtomicHWID* GetAtomicHWID() { return m_pAtomicHWID; }

    void StartBasicChecks();

    bool RunScanners() { return m_bRunScanners; }
    void RunScanners(bool bRun) { m_bRunScanners = bRun; }

    void NotifyDetection(eDetectionType DetectionType, std::unordered_map<std::string, ArgType> kwargs = {}, bool bTakeScreenshot = true);

    void ForceHardKick(eHardKickReason KickReason, std::string strOptionalDescription = "");

    void TerminateThreads();
    void Shutdown(std::string strReason = "AntiCheat Shutdown", bool bTerminateThreads = true, bool bUnload = true);

    HANDLE GetProcessHandle() { return m_hProcess; }
    int    GetProcessID() { return m_iTargetProcessID; }

    bool IsValidProcessHandle() { return m_hProcess != NULL && m_hProcess != INVALID_HANDLE_VALUE; }

    bool                         IsAtomicThread(HANDLE hThread);
    std::vector<CAtomicThread*>& GetAtomicThreads() { return m_vAtomicThreads; }

private:
    void        SetupExitHandlers();
    static void TerminateSignalHandler(int signal);
    static BOOL ConsoleHandler(DWORD dwSignal);

    bool    m_bAlive;
    int     m_iTargetProcessID;
    HANDLE  m_hProcess;

    CAtomicHWID*   m_pAtomicHWID;
    jsoncons::json m_HWIDCache;

    CAtomicNetwork* m_pAtomicNetwork;
    CGuardManager*  m_pGuardManager;

    std::vector<eDetectionType> m_vDetectedTypes;
    std::vector<CAtomicThread*> m_vAtomicThreads;
    LPVOID                      m_lpAntiCheatModuleBase;
    bool                        m_bRunScanners;

    KernelCalls_OBJECT_ATTRIBUTES m_ObjAttr{};
    KernelCalls_CLIENT_ID         m_ClientId{};
};

extern CAtomicAntiCheat* g_pAtomicAntiCheat;