#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CMemoryGuard final : public CGuardBase
{
public:
    CMemoryGuard();
    ~CMemoryGuard();

    void DoPulse() override;
};
