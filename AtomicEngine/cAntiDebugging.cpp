#include "cAntiDebugging.hpp"

AntiDebug g_AntiDebug;

void AntiDebug::CheckForDebugger()
{
    const int MonitorLoopDelayMS = 1000;

    while (Monitoring)
    {
        CheckHardwareDebugRegisters();

        RunDetectionFunctions();

        if (IsDBK64DriverLoaded())
        {
            OnDebuggerDetected(DetectionFlags::DEBUG_DBK64_DRIVER);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(MonitorLoopDelayMS));
    }
}

void AntiDebug::CheckHardwareDebugRegisters()
{
    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD currentProcessID = GetCurrentProcessId();

    if (Thread32First(hThreadSnap, &te32))
    {
        do
        {
            if (te32.th32OwnerProcessID == currentProcessID && te32.th32ThreadID != GetCurrentThreadId())
            {
                HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);

                if (hThread == NULL)
                {
                    continue;
                }

                SuspendThread(hThread);

                CONTEXT context;
                context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

                if (GetThreadContext(hThread, &context))
                {
                    if (context.Dr0 || context.Dr1 || context.Dr2 || context.Dr3 || context.Dr6 || context.Dr7)
                    {
                        ResumeThread(hThread);
                        CloseHandle(hThreadSnap);
                        CloseHandle(hThread);

                        // OnDebuggerDetected(DetectionFlags::DEBUG_HARDWARE_REGISTERS);
                        return;
                    }
                }

                ResumeThread(hThread);
                CloseHandle(hThread);
            }
        } while (Thread32Next(hThreadSnap, &te32));
    }

    CloseHandle(hThreadSnap);
}

bool AntiDebug::PreventWindowsDebuggers()
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");

    if (!ntdll)
    {
        return false;
    }

    DWORD dwOldProt = 0;

    UINT64 DbgBreakpoint_Address = (UINT64)GetProcAddress(ntdll, "DbgBreakPoint");
    UINT64 DbgUiRemoteBreakin_Address = (UINT64)GetProcAddress(ntdll, "DbgUiRemoteBreakin");

    if (DbgBreakpoint_Address)
    {
        if (VirtualProtect((LPVOID)DbgBreakpoint_Address, 1, PAGE_EXECUTE_READWRITE, &dwOldProt))
        {
            __try
            {
                *(BYTE*)DbgBreakpoint_Address = 0xC3;            // RET instruction
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }

            VirtualProtect((LPVOID)DbgBreakpoint_Address, 1, dwOldProt, &dwOldProt);
        }
    }

    if (DbgUiRemoteBreakin_Address)
    {
        if (VirtualProtect((LPVOID)DbgUiRemoteBreakin_Address, 1, PAGE_EXECUTE_READWRITE, &dwOldProt))
        {
            __try
            {
                *(BYTE*)DbgUiRemoteBreakin_Address = 0xC3;            // RET instruction
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return false;
            }

            VirtualProtect((LPVOID)DbgUiRemoteBreakin_Address, 1, dwOldProt, &dwOldProt);
        }
    }

    return true;
}

bool AntiDebug::HideThreadFromDebugger(HANDLE hThread)
{
    typedef NTSTATUS(NTAPI * pNtSetInformationThread)(HANDLE, UINT, PVOID, ULONG);
    NTSTATUS Status;

    pNtSetInformationThread NtSetInformationThread = (pNtSetInformationThread)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread");

    if (NtSetInformationThread == NULL)
        return false;

    if (hThread == NULL)
        Status = NtSetInformationThread(GetCurrentThread(), 0x11, 0, 0);            // ThreadHideFromDebugger
    else
        Status = NtSetInformationThread(hThread, 0x11, 0, 0);

    return (Status == 0);
}

bool AntiDebug::IsDBK64DriverLoaded()
{
    // Simple driver detection - you can enhance this with more sophisticated checks
    HANDLE hDevice = CreateFileA("\\\\.\\DBK64", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hDevice);
        return true;
    }
    return false;
}

void AntiDebug::HideAllThreadsFromDebugger()
{
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return;
    }

    DWORD pid = GetCurrentProcessId();

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &te))
    {
        do
        {
            if (te.th32OwnerProcessID == pid)
            {
                HANDLE hThread = OpenThread(THREAD_SET_INFORMATION | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);

                if (!hThread)
                {
                    continue;
                }

                HideThreadFromDebugger(hThread);
                CloseHandle(hThread);
            }
        } while (Thread32Next(hSnapshot, &te));
    }

    CloseHandle(hSnapshot);
}
