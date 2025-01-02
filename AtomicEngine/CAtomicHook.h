#pragma once
#include "StdInc.h"

class CAtomicHook
{
public:
    CAtomicHook(LPVOID lpTargetFunction, LPVOID lpDetourFunction);
    ~CAtomicHook();
    
    static CAtomicHook* Create(LPVOID lpTargetFunction, LPVOID lpDetourFunction);

    void Enable();
    void Disable();

private:
    void ReadOriginalBytes();
    void CreateTrampoline();

    bool m_bEnabled;

    LPVOID m_lpOriginalFunction;
    LPVOID m_lpTargetFunction;
    LPVOID m_lpDetourFunction;

    std::vector<BYTE> m_vOriginalBytes;
};
