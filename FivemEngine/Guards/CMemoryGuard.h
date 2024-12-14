#pragma once
#include "StdInc.h"
#include "CGuardBase.h"

class CMemoryGuard : public CGuardBase
{
public:
    CMemoryGuard();
    ~CMemoryGuard();

    void Initialize() override;
    void DoPulse() override;

private:
    MEMORY_BASIC_INFORMATION m_mbi;
    SYSTEM_INFO              m_systemInfo;
    DWORD64                  m_dwMaxAddress;
    DWORD64                  m_dwCurrentAddress;
};
