#pragma once
#include "StdInc.h"
#include "Guards/CMemoryGuard.h"
#include "Guards/CHeuristicGuard.h"

class CGuardManager
{
public:
    CGuardManager();
    ~CGuardManager();

    void InitializeGuards();
    void StartPulse(CGuardManager* pGuardManager);

    CMemoryGuard* GetMemoryGuard() { return m_pMemoryGuard; }
    CHeuristicGuard* GetHeuristicGuard() { return m_pHeuristicGuard; }

private:
    CMemoryGuard*            m_pMemoryGuard;
    CHeuristicGuard*         m_pHeuristicGuard;
    std::vector<std::thread> m_threads;
};
