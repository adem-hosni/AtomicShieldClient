#include "StdInc.h"

CThreadGuard::CThreadGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
    m_dwCurrentAddress = NULL;
    m_dwMaxAddress = NULL;
}

CThreadGuard::~CThreadGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
}

void CThreadGuard::DoPulse()
{
    while (true)
    {
        THREADENTRY32 th32;
        HANDLE        hSnapshot = NULL;
        th32.dwSize = sizeof(THREADENTRY32);
        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (Thread32First(hSnapshot, &th32))
        {
            do
            {
                if (th32.th32OwnerProcessID == GetCurrentProcessId() && th32.th32ThreadID != GetCurrentThreadId())
                {
                    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, th32.th32ThreadID);
                    if (hThread)
                    {
                        DWORD64 dwTempoaryBase = NULL;
                        NtQueryInformationThread(hThread, (THREADINFOCLASS)9, &dwTempoaryBase, sizeof(DWORD64), NULL);
                        DWORD64 dwModuleBase = Utils::IsAddressInModuledRange(dwTempoaryBase);
                        if (dwModuleBase == NULL)
                        {
                            MEMORY_BASIC_INFORMATION mbi;
                            SMemoryDetectionReport   Report;

                            VirtualQuery((LPCVOID)dwTempoaryBase, &mbi, sizeof(th32.dwSize));

                            Report.AllocatedBase = mbi.AllocationBase;
                            Report.AllocatedProtect = mbi.AllocationProtect;
                            Report.RegionSize = mbi.RegionSize;
                            char szFilePath[MAX_PATH];

                            GetModuleFileName((HMODULE)dwModuleBase, szFilePath, MAX_PATH);

                            SharedUtil::AddDebugLog("detected thread at 0x%x, spawned from 0x%x possible module name: %s", dwTempoaryBase, dwModuleBase, szFilePath);

                            g_pSafeAntiCheat->NotifyDetection(eDetectionType::UNAUTHORIZED_THREAD, &Report);
                        }
                    }
                    CloseHandle(hThread);
                }
            } while (Thread32Next(hSnapshot, &th32));
        }
        Sleep(1000);
    }
}