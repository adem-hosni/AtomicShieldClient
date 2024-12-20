#include "StdInc.h"

CGuardManager::CGuardManager()
{
    m_pMemoryGuard = new CMemoryGuard();
    m_pHeuristicGuard = new CHeuristicGuard();
    m_pModuleGuard = new CModuleGuard();

    m_threads.clear(); // Ensure no memory leaks
}

CGuardManager::~CGuardManager()
{
    if (m_pMemoryGuard)
        delete[] m_pMemoryGuard;
}

void CGuardManager::InitializeGuards()
{
    m_pMemoryGuard->Initialize();
}

void CGuardManager::StartPulse(CGuardManager* pGuardManager)
{
    _beginthread((_beginthread_proc_type)CThreadGuard::StaticPulse, NULL, this);
    _beginthread((_beginthread_proc_type)CMemoryGuard::StaticPulse, NULL, this);
    _beginthread((_beginthread_proc_type)CModuleGuard::StaticPulse, NULL, this);
    _beginthread((_beginthread_proc_type)CHeuristicGuard::StaticPulse, NULL, this);
}