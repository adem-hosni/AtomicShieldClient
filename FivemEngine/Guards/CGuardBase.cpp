#include "StdInc.h"

void CGuardBase::Initialize()
{
    GetNativeSystemInfo(&m_systemInfo);
    m_dwMaxAddress = (DWORD64)m_systemInfo.lpMaximumApplicationAddress - (DWORD64)m_systemInfo.lpMinimumApplicationAddress;
}

void CGuardBase::StaticPulse(void* pContext)
{
    CGuardBase* pGuard = reinterpret_cast<CGuardBase*>(pContext);
    pGuard->DoPulse();
}

void CGuardBase::DoPulse()
{
}