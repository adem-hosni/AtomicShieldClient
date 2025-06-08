#pragma once
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

class CGuardBase
{
public:
    virtual void Initialize();
    static void  StaticPulse(void* pContext);
    virtual void DoPulse();
    virtual void ClearDetections();
};
