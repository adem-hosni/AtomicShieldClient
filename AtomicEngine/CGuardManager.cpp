#include "StdInc.h"

CGuardManager::CGuardManager()
{
    m_pHeuristicGuard = new CHeuristicGuard();
    m_pModuleGuard = new CModuleGuard();
    m_pProcessGuard = new CProcessGuard();

    m_threads.clear();            // Ensure no memory leaks
}

CGuardManager::~CGuardManager()
{
}

void CGuardManager::InitializeGuards()
{
    //m_pModuleGuard->Initialize();
    m_pHeuristicGuard->Initialize();
}

void CGuardManager::StartPulse(CGuardManager* pGuardManager)
{
    SharedUtil::AddDebugLog("calling heuristic guard pulse");
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)CHeuristicGuard::StaticPulse, m_pHeuristicGuard, 0, 0);
   // CAtomicThread::Create(CHeuristicGuard::StaticPulse, m_pHeuristicGuard);
    //CAtomicThread::Create(CProcessGuard::StaticPulse, m_pProcessGuard);
    //CAtomicThread::Create(CModuleGuard::StaticPulse, m_pModuleGuard);
}