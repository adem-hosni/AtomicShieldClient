#pragma once
#include "StdInc.h"
#include "CSafeNetwork.h"
#include "STiming.h"
#include "CGuardManager.h"

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

    HANDLE GetProcessHandle() { return m_hProcess; }
    int    GetProcessID() { return m_iTargetProcessID; }

private:
    int     m_iTargetProcessID;
    HANDLE  m_hProcess;
    STiming m_Timing;

    jsoncons::json m_HWIDCache;

    CSafeNetwork*  m_pSafeNetwork;
    CGuardManager* m_pGuardManager;
};

extern CSafeAntiCheat* g_pSafeAntiCheat;