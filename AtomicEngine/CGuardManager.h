#pragma once
#include "StdInc.h"
#include "Guards/CHeuristicGuard.h"
#include "Guards/CModuleGuard.h"
#include "Guards/CProcessGuard.h"
#include "Guards/CManualMappingGuard.h"

class CGuardManager
{
public:
    CGuardManager();
    ~CGuardManager();

    void InitializeGuards();
    void StartGuards();
    void DoPulse();
    void StopGuards();

    void ClearDetections();

    CProcessGuard*   GetProcessGuard() { return m_pProcessGuard; }
    CHeuristicGuard* GetHeuristicGuard() { return m_pHeuristicGuard; }
    CManualMappingGuard* GetManualMappingGuard() { return m_pManualMappingGuard; }


    bool IsPulseStarted() { return m_bPulseStarted; }

private:
    CHeuristicGuard* m_pHeuristicGuard;
    CProcessGuard* m_pProcessGuard;
    CManualMappingGuard* m_pManualMappingGuard;

    std::vector<CAtomicThread*> m_vThreads;
    bool                        m_bPulseStarted;
};
