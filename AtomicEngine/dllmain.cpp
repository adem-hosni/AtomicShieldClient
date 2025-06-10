#include "StdInc.h"
#include "SharedChecks.h"
#include <winternl.h>

BOOL AdjustTokenPrivilege(const HANDLE hproc)
{
    HANDLE htoken;
    DWORD  dw_t;

    if (OpenProcessToken(hproc, (((0x00020000L)) | (0x0008)) | (0x0008) | (0x0020), &htoken))
    {
        DWORD                   dw_s = sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES) * 100;
        std::unique_ptr<BYTE[]> memory = std::make_unique<BYTE[]>(dw_s);

        if (memory)
        {
            TOKEN_PRIVILEGES* priv = reinterpret_cast<TOKEN_PRIVILEGES*>(memory.get());
            if (GetTokenInformation(htoken, TokenPrivileges, priv, dw_s, &dw_t))
            {
                if (priv->PrivilegeCount > 0)
                {
                    for (DWORD i = 0; i < priv->PrivilegeCount; i++)
                    {
                        priv->Privileges[i].Attributes = 0x00000002L;
                    }

                    if (AdjustTokenPrivileges(htoken, 0, priv, dw_s, 0, 0))
                    {
                        CloseHandle(htoken);
                        return 1;
                    }
                }
            }
        }
        CloseHandle(htoken);
    }

    return 0;
}

void EntryPoint(LPVOID lpAntiCheatModuleBase)
{
    SharedUtil::AddDebugLog(
        "===================================== AtomicShield AntiCheat Loaded! "
        "=====================================\n");
    SharedUtil::SetRegistryIntValue("AtomicShield", 1);

    if (g_pAtomicAntiCheat->Initialize())
    {
        AdjustTokenPrivilege(GetCurrentProcess());
        SharedUtil::AddDebugLog("Starting Basic Checks...");
        g_pAtomicAntiCheat->StartBasicChecks();
        SharedUtil::AddDebugLog("End Basic Checks");
    }
    _beginthread((_beginthread_proc_type)CAtomicAntiCheat::StaticPulse, NULL, g_pAtomicAntiCheat);
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
