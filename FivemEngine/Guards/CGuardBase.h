#pragma once
class CGuardBase
{
public:
    virtual void Initialize();
    static void  StaticPulse(void* pContext);
    virtual void DoPulse();

protected:
    MEMORY_BASIC_INFORMATION m_mbi;
    SYSTEM_INFO              m_systemInfo;
    DWORD64                  m_dwMaxAddress;
    DWORD64                  m_dwCurrentAddress;
};
