#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CMemoryGuard final : public CGuardBase
{
public:
    CMemoryGuard();
    ~CMemoryGuard();

    static void StaticPulse(void* pContext) { reinterpret_cast<CMemoryGuard*>(pContext)->DoPulse(); }
    void DoPulse() override;
};
