#pragma once
#include "StdInc.h"
#include "CGuardBase.h"
#include <winternl.h>


class CModuleGuard final : public CGuardBase
{
public:
    CModuleGuard();
    ~CModuleGuard();
    static void StaticPulse(void* pContext) { reinterpret_cast<CModuleGuard*>(pContext)->DoPulse(); }

    void DoPulse() override;


    VOID HookLoadDll(LPVOID lpAddr);
    NTSTATUS __stdcall _LdrLoadDll(PWSTR SearchPath OPTIONAL, PULONG DllCharacteristics OPTIONAL, PUNICODE_STRING DllName, PVOID* BaseAddress);
    std::vector<const char*> cAllowDlls;
    typedef void(WINAPI* LdrLoadDll_)(PWSTR SearchPath OPTIONAL, PULONG DllCharacteristics OPTIONAL, PUNICODE_STRING DllName, PVOID* BaseAddress);

    LPVOID lpAddr;
    CHAR   OriginalBytes[50];
};
