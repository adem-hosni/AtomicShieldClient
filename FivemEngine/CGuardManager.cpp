#include "StdInc.h"

CGuardManager::CGuardManager()
{
    m_pMemoryGuard = new CMemoryGuard();
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

void CGuardManager::StartThreads()
{
    m_threads.emplace_back(&CMemoryGuard::DoPulse, m_pMemoryGuard);
}