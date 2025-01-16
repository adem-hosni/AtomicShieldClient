#pragma once
#include "StdInc.h"
#include "CGuardBase.h"
#include <winternl.h>

class CModuleGuard final : public CGuardBase
{
public:
    CModuleGuard();
    ~CModuleGuard();

    void __stdcall Initialize() override;
    static void StaticPulse(void* pContext) { reinterpret_cast<CModuleGuard*>(pContext)->DoPulse(); }
    void        DoPulse() override;

    std::vector<const wchar_t*> m_vAllowedModules;
    typedef void(WINAPI* LdrLoadDll_)(PWSTR SearchPath OPTIONAL, PULONG DllCharacteristics OPTIONAL, PUNICODE_STRING DllName, PVOID* BaseAddress);

    LPVOID lpAddr;
    CHAR   OriginalBytes[50];
};
