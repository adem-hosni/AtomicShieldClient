#pragma once
#include "StdInc.h"
#include "Guards/CMemoryGuard.h"

class CGuardManager
{
public:
    CGuardManager();
    ~CGuardManager();

    void InitializeGuards();
    void StartThreads();

    CMemoryGuard* GetMemoryGuard() { return m_pMemoryGuard; }

private:
    CMemoryGuard* m_pMemoryGuard;
    std::vector<std::thread> m_threads;
};
