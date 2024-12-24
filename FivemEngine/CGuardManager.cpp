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

    CAtomicThread::Create(CThreadGuard::StaticPulse, m_pThreadGuard);
    CAtomicThread::Create(CMemoryGuard::StaticPulse, m_pMemoryGuard);
    CAtomicThread::Create(CModuleGuard::StaticPulse, m_pModuleGuard);
    CAtomicThread::Create(CHeuristicGuard::StaticPulse, m_pHeuristicGuard);
}