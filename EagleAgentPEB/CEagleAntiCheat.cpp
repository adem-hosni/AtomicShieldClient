#include "CEagleAntiCheat.h"
#include "SharedUtil.h"

CEagleAntiCheat* g_pEagleAntiCheat = new CEagleAntiCheat();

CEagleAntiCheat::CEagleAntiCheat()
{
    m_pEagleNetwork = new CEagleNetwork();
}

CEagleAntiCheat::~CEagleAntiCheat()
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

bool CEagleAntiCheat::CheckGameAntiCheatsStatus()
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

