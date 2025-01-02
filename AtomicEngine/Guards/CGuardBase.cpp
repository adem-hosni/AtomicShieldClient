#include "StdInc.h"

void CGuardBase::Initialize()
{
   
}

void CGuardBase::StaticPulse(void* pContext)
{
    CGuardBase* pGuard = reinterpret_cast<CGuardBase*>(pContext);
    pGuard->DoPulse();
}

void CGuardBase::DoPulse()
{
}