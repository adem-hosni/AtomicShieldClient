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
    __security_init_cookie();

    SharedUtil::AddDebugLog("AtomicShield: DLL Loaded!");
    g_pAtomicLogger->Initialize();

    SharedLogger::LogInfo(
        "===================================== AtomicShield AntiCheat Loaded! "
        "=====================================");

    SharedUtil::SetRegistryIntValue("AtomicShield", "AtomicShield", 1);


    CAntiDebugging* pAntiDebugging = new CAntiDebugging(
        [](eDebugDetectionFlags DetectionFlag, std::string strReason) -> void*
        {
            if (DetectionFlag == eDebugDetectionFlags::NONE || DetectionFlag == eDebugDetectionFlags::EXECUTION_ERROR)
                return nullptr;
            char szReason[256];
            memset(szReason, 0, sizeof(szReason));
            sprintf(szReason, "%s (%d)", strReason.c_str(), (int)DetectionFlag);
            g_pAtomicAntiCheat->Shutdown(szReason);
            return nullptr;
        });
    pAntiDebugging->StartPulse();

    CLatencyEvaluator::SetupServerEndPoint([](std::string strBestEndPoint) -> void { g_pAtomicAntiCheat->GetNetwork()->SetServerEndPoint(strBestEndPoint); },
                                           false);

    EnableDebugPrivilege();
    g_pAtomicAntiCheat->Initialize();


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
            break;
        }
        case DLL_THREAD_ATTACH:
            SharedUtil::AddDebugLog("AtomicShield: DLL_THREAD_ATTACH");
            break;
        case DLL_THREAD_DETACH:
            SharedUtil::AddDebugLog("AtomicShield: DLL_THREAD_DETACH");
            break;
        case DLL_PROCESS_DETACH:
            SharedUtil::AddDebugLog("AtomicShield: DLL_PROCESS_DETACH");
            break;
    }
    return TRUE;
}