#include "StdInc.h"
#include "SharedChecks.h"
#include <winternl.h>
#include "SharedProtocols.h"
#include "CLatencyEvaluator.h"

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

bool EnableDebugPrivilege()
{
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    LUID luid;
    if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &luid))
    {
        CloseHandle(hToken);
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    bool  success = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    DWORD err = GetLastError();
    CloseHandle(hToken);

    return success && (err == ERROR_SUCCESS);
}

DWORD WINAPI EntryPoint(LPVOID lpAntiCheatModuleBase)
{
    SharedUtil::AddDebugLog(
        "===================================== AtomicShield AntiCheat Loaded! " CLIENT_BUILD_TIMESTAMP 
        " =====================================\n");

    SharedUtil::SetRegistryIntValue("AtomicShield", "AtomicShield", 1);

    SharedUtil::AddDebugLog("AtomicShield: After SetRegistryIntValue");

    CAntiDebugging* pAntiDebugging = new CAntiDebugging(
        [](eDebugDetectionFlags DetectionFlag, std::string strReason) -> void*
        {
            if (DetectionFlag == eDebugDetectionFlags::NONE || DetectionFlag == eDebugDetectionFlags::EXECUTION_ERROR)
            {
                SharedUtil::AddDebugLog("AntiDebugging: DetectionCallback called with %s and reason: \"%s\"", DetectionFlag == eDebugDetectionFlags::NONE ? "NONE" : "EXECUTION_ERROR", strReason.c_str());
                return nullptr;
            }

            char szReason[256];
            memset(szReason, 0, sizeof(szReason));
            sprintf(szReason, "%s (%d)", strReason.c_str(), (int)DetectionFlag);

            g_pAtomicAntiCheat->ForceHardKick(eHardKickReason::DEBUGGER_DETECTED, szReason);
            g_pAtomicAntiCheat->Shutdown(szReason);
            return nullptr;
        });
    pAntiDebugging->StartPulse();

    SharedUtil::AddDebugLog("AtomicShield: After AntiDebugging setup");

    // SharedProtocols::EnableProcessMitigations();

    CLatencyEvaluator::SetupServerEndPoint(
        [](std::string strBestEndPoint) -> void
        {
            g_pAtomicAntiCheat->GetNetwork()->SetServerEndPoint(strBestEndPoint);
        },
                                           false);

    EnableDebugPrivilege();
    g_pAtomicAntiCheat->Initialize();

    SharedUtil::AddDebugLog("AtomicShield: After g_pAtomicAntiCheat->Initialize");

    CAtomicThread::Create(CAtomicAntiCheat::StaticPulse, g_pAtomicAntiCheat);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        {
            CreateThread(nullptr, 0, EntryPoint, hModule, 0, nullptr);
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}