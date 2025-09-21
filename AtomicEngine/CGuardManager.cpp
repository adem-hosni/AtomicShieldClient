#include "StdInc.h"

CGuardManager::CGuardManager()
{
    m_pHeuristicGuard = new CHeuristicGuard();
    // m_pModuleGuard = new CModuleGuard();
    m_pProcessGuard = new CProcessGuard();
    m_bPulseStarted = false;

    m_vThreads.clear();            // Ensure no memory leaks
}

CGuardManager::~CGuardManager()
{
}

void CGuardManager::InitializeGuards()
{
    m_pHeuristicGuard->Initialize();
    m_pManualMappingGuard->Initialize();
}

void CGuardManager::StartGuards()
{
    SharedUtil::AddDebugLog("Starting threads");
    m_bPulseStarted = true;
    m_vThreads.push_back(CAtomicThread::Create(CProcessGuard::StaticPulse, m_pProcessGuard));
    m_vThreads.push_back(CAtomicThread::Create(CHeuristicGuard::StaticPulse, m_pHeuristicGuard));
    // m_vThreads.push_back(CAtomicThread::Create(CManualMappingGuard::StaticPulse, m_pManualMappingGuard));
}

void CGuardManager::DoPulse()
{
}

void CGuardManager::StopGuards()
{
    SharedUtil::AddDebugLog("Stopping threads");
    m_bPulseStarted = false;

    for (const auto& thread : m_vThreads)
    {
        thread->Terminate();
    }
}

void CGuardManager::ClearDetections()
{
    m_pProcessGuard->ClearDetections();
    m_pHeuristicGuard->ClearDetections();
}