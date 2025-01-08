#pragma once
#include "StdInc.h"
#include "Guards/CMemoryGuard.h"
#include "Guards/CHeuristicGuard.h"
#include "Guards/CThreadGuard.h"
#include "Guards/CModuleGuard.h"
#include "Guards/CProcessGuard.h"

class CGuardManager
{
public:
    CGuardManager();
    ~CGuardManager();

    void InitializeGuards();
    void StartPulse(CGuardManager* pGuardManager);

    CProcessGuard*   GetProcessGuard() { return m_pProcessGuard; }
    CMemoryGuard*    GetMemoryGuard() { return m_pMemoryGuard; }
    CHeuristicGuard* GetHeuristicGuard() { return m_pHeuristicGuard; }
    CThreadGuard*    GetThreadGuard() { return m_pThreadGuard; }
    CModuleGuard*    GetModuleGuard() { return m_pModuleGuard; }

private:
    CMemoryGuard*    m_pMemoryGuard;
    CHeuristicGuard* m_pHeuristicGuard;
    CThreadGuard*    m_pThreadGuard;
    CModuleGuard*    m_pModuleGuard;
    CProcessGuard*   m_pProcessGuard;

    std::vector<std::thread> m_threads;
};
