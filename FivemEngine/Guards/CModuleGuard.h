#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CModuleGuard final : public CGuardBase
{
public:
    CModuleGuard();
    ~CModuleGuard();
    static void StaticPulse(void* pContext) { reinterpret_cast<CModuleGuard*>(pContext)->DoPulse(); }

    void DoPulse() override;
};
