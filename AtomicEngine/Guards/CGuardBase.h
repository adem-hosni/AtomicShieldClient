#pragma once
class CGuardBase
{
public:
    virtual void Initialize();
    static void  StaticPulse(void* pContext);
    virtual void DoPulse();
};
