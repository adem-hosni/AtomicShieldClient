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
    static NTSTATUS __stdcall _LdrLoadDll(PWSTR SearchPath OPTIONAL, PULONG DllCharacteristics OPTIONAL, PUNICODE_STRING DllName, PVOID* BaseAddress);
    #define dwAllowDllCount 1
    static constexpr CHAR cAllowDlls[1][MAX_PATH] = {"C:\\Windows\\System32\\rsaenh.dll"};
    typedef void(WINAPI* LdrLoadDll_)(PWSTR SearchPath OPTIONAL, PULONG DllCharacteristics OPTIONAL, PUNICODE_STRING DllName, PVOID* BaseAddress);

    static LPVOID lpAddr;
    static CHAR   OriginalBytes[50];
};
