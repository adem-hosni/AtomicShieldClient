#pragma once
#include "StdInc.h"
#include "Guards/CHeuristicGuard.h"
#include "Guards/CModuleGuard.h"
#include "Guards/CProcessGuard.h"

class CGuardManager
{
public:
    CGuardManager();
    ~CGuardManager();

    void InitializeGuards();
    void StartPulse();
    void StopPulse();

    CProcessGuard*   GetProcessGuard() { return m_pProcessGuard; }
    CHeuristicGuard* GetHeuristicGuard() { return m_pHeuristicGuard; }
    // CModuleGuard*    GetModuleGuard() { return m_pModuleGuard; }

    bool IsPulseStarted() { return m_bPulseStarted; }

private:
    CHeuristicGuard* m_pHeuristicGuard;
    // CModuleGuard*    m_pModuleGuard;
    CProcessGuard* m_pProcessGuard;

    std::vector<CAtomicThread*> m_vThreads;
    bool                        m_bPulseStarted;
};
