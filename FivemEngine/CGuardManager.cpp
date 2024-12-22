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
    SharedUtil::AddDebugLog("Start pulse");
    _beginthread((_beginthread_proc_type)CThreadGuard::StaticPulse, NULL, m_pThreadGuard);
    _beginthread((_beginthread_proc_type)CMemoryGuard::StaticPulse, NULL, m_pMemoryGuard);
    _beginthread((_beginthread_proc_type)CModuleGuard::StaticPulse, NULL, m_pModuleGuard);
    //_beginthread((_beginthread_proc_type)CHeuristicGuard::StaticPulse, NULL, m_pHeuristicGuard);
}