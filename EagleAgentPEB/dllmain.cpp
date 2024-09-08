#include "StdInc.h"

void EntryPoint(LPVOID lpThreadParameter)
{
    if (!g_pEagleNetwork->Connect())
    {
        MessageBox(0, "Failed to connect to the server", "Error", 0);
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:        
        _beginthread((_beginthread_proc_type)EntryPoint, NULL, lpReserved);

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

