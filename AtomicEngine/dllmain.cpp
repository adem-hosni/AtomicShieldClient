#include "StdInc.h"
#include "SharedChecks.h"
#include <winternl.h>
#include "PEBHide.h"

void EntryPoint(LPVOID lpAntiCheatModuleBase)
{
#ifdef _DEBUG
    //AllocConsole();
    //freopen("CONIN$", "r", stdin);
    //freopen("CONOUT$", "w", stdout);
    //freopen("CONOUT$", "w", stderr);
    
   // _beginthread((_beginthread_proc_type)SharedChecks::CheckProcessList, NULL, SharedChecks::MaliciousProcessAlert);
#endif
    //PEBHide::EraseSelfPEHeader(lpAntiCheatModuleBase);
    //PEBHide::UnlinkSelfLdrModule(lpAntiCheatModuleBase);

    // SharedProtocols::EnableProcessMitigations(true, true, true, true, true);

    if (g_pAtomicAntiCheat->Initialize())
    {
        g_pAtomicAntiCheat->SetAntiCheatModuleBase(lpAntiCheatModuleBase);
        g_pAtomicAntiCheat->StartBasicChecks();
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            _beginthread((_beginthread_proc_type)EntryPoint, NULL, hModule);
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
