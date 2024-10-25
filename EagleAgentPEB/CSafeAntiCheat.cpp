#include "CSafeAntiCheat.h"
#include "SharedUtil.h"
#include <ctime>

CSafeAntiCheat* g_pSafeAntiCheat = new CSafeAntiCheat();

CSafeAntiCheat::CSafeAntiCheat()
{
    m_pEagleNetwork = new CSafeNetwork();
    m_Timing = {};
}

CSafeAntiCheat::~CSafeAntiCheat()
{
    if (m_pEagleNetwork)
        delete m_pEagleNetwork;
}

bool CheckFairplayStatus(SC_HANDLE hFairplayService)
{
    if (!hFairplayService)
        return false;

    SERVICE_STATUS_PROCESS status;
    DWORD                  dwNeededBytes;

    if (!QueryServiceStatusEx(hFairplayService, SC_STATUS_PROCESS_INFO, (LPBYTE)&status, sizeof(SERVICE_STATUS_PROCESS), &dwNeededBytes))
        return false;

    if (status.dwCurrentState == SERVICE_STOP_PENDING || status.dwCurrentState == SERVICE_STOPPED || status.dwCurrentState == SERVICE_STOP_PENDING ||
        status.dwCurrentState == SERVICE_PAUSED)
    {
        if (SharedUtil::GetProcessID("gta_sa.exe"))
            return false;
    }
    return true;
}

bool CSafeAntiCheat::CheckGameAntiCheatsStatus()
{
    SC_HANDLE hServiceControl = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (!hServiceControl)
        return false;

    const char* szServiceName = "Fairplay";
    SC_HANDLE   hFairplayService = OpenService(hServiceControl, szServiceName, SERVICE_QUERY_STATUS);

    if (!hFairplayService)
    {
        CloseHandle(hFairplayService);

        for (int i = 1; i <= 4; i++)
        {
            char szNewServiceName[32];
            memset(szNewServiceName, 0, sizeof(szNewServiceName));
            sprintf(szNewServiceName, "%s%d", szServiceName, i);

            hFairplayService = OpenService(hServiceControl, szServiceName, SERVICE_QUERY_STATUS);

            if (CheckFairplayStatus(hFairplayService))
            {
                CloseHandle(hFairplayService);
                CloseHandle(hServiceControl);
                return true;
            }
        }
    }
    else
    {
        bool bResult = CheckFairplayStatus(hFairplayService);
        CloseHandle(hFairplayService);
        CloseHandle(hServiceControl);
        return bResult;
    }

    CloseHandle(hFairplayService);
    CloseHandle(hServiceControl);
    return false;
}

void CSafeAntiCheat::DoPulse()
{
    while (true)
    {
        while (!SharedUtil::GetProcessID("gta_sa.exe"))
            Sleep(100);

        long long llCurrentTime = time(NULL);
        CSafeNetwork* pEagleNetwork = g_pSafeAntiCheat->GetEagleNetwork();
        STiming&   Timing = g_pSafeAntiCheat->GetTiming();

        if (!g_pMemoryScanner->IsAttached())
            g_pMemoryScanner->Attach(SharedUtil::GetProcessID("gta_sa.exe"));

        if (llCurrentTime - Timing.llLastGameAntiCheatCheck > GAME_ANTICHEAT_STATUS_CHECK_INTERVAL)
        {
            jsoncons::json JsonRequest = jsoncons::json::object();
            JsonRequest["status"] = g_pSafeAntiCheat->CheckGameAntiCheatsStatus();
            pEagleNetwork->SendPacket(eEaglePacketID::GAME_ANTICHEAT_COMPONENT_STATUS, JsonRequest);
            Timing.llLastGameAntiCheatCheck = llCurrentTime;
        }

        if (llCurrentTime - Timing.llLastMemoryScan > GAME_MEMORY_SCAN_INTERVAL)
        {
            g_pMemoryScanner->ScanStrings(pEagleNetwork->GetSignatures());

            std::vector<std::string> vSignatures = g_pMemoryScanner->GetDetectedSignatures();
            unsigned int             uiScanResult = vSignatures.size();

            // New Signature Found ?
            if (uiScanResult != g_pMemoryScanner->GetLatestScanResult())
            {
                g_pMemoryScanner->UpdateLatestScanResult(uiScanResult);
                jsoncons::json RequestData = jsoncons::json::object();
                RequestData["signatures"] = vSignatures;
                pEagleNetwork->SendPacket(eEaglePacketID::MALICIOUS_SIGNATURE_DETECTION, RequestData);
            }
            Timing.llLastMemoryScan = llCurrentTime;
        }
    }
}