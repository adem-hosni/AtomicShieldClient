#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CThreadGuard final : public CGuardBase
{
public:
    CThreadGuard();
    ~CThreadGuard();
    static void StaticPulse(void* pContext) { reinterpret_cast<CThreadGuard*>(pContext)->DoPulse(); }

    void DoPulse() override;
};
