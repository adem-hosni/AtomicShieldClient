#include "StdInc.h"
#include "SharedChecks.h"



void EntryPoint(LPVOID lpThreadParameter)
{
    //_beginthread((_beginthread_proc_type)SharedChecks::CheckProcessList, NULL, SharedChecks::MaliciousProcessAlert);

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    g_pSafeAntiCheat->Initialize();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:        
    {
        _beginthread((_beginthread_proc_type)EntryPoint, NULL, lpReserved);
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

