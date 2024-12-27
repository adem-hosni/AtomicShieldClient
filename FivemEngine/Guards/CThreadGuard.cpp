#include "StdInc.h"

CThreadGuard::CThreadGuard()
{
}

CThreadGuard::~CThreadGuard()
{
}

void CThreadGuard::DoPulse()
{
    while (true)
    {
        int           iMaliciousThreadCount = 0;
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
                        if (!g_pSafeAntiCheat->IsAtomicThread(hThread))
                        {
                            DWORD64 dwTempoaryBase = NULL;
                            NtQueryInformationThread(hThread, (THREADINFOCLASS)9, &dwTempoaryBase, sizeof(DWORD64), NULL);
                            DWORD64 dwModuleBase = Utils::IsAddressInModuledRange(dwTempoaryBase);
                            if (dwModuleBase == -1)
                            {
                                iMaliciousThreadCount++;
                                if (iMaliciousThreadCount > 1)
                                {
                                    MEMORY_BASIC_INFORMATION mbi;
                                    SMemoryDetectionReport   Report;

                                    VirtualQuery((LPCVOID)dwTempoaryBase, &mbi, sizeof(th32.dwSize));

                                    Report.AllocatedBase = mbi.AllocationBase;
                                    Report.AllocatedProtect = mbi.AllocationProtect;
                                    Report.RegionSize = mbi.RegionSize;

                                    SharedUtil::AddDebugLog("Detected Malicious Thread at 0x%x", dwTempoaryBase);
                                    g_pSafeAntiCheat->NotifyDetection(eDetectionType::UNAUTHORIZED_THREAD, {{"allocated_base", (DWORD64)mbi.AllocationBase},
                                                                       {"allocated_protect", (DWORD64)mbi.AllocationProtect},
                                                                       {"region_size", (DWORD64)mbi.RegionSize}});
                                }
                            }
                        }
                        CloseHandle(hThread);
                    }
                }
            } while (Thread32Next(hSnapshot, &th32));
        }
        Sleep(1000);
    }
}