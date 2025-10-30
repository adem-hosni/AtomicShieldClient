#include "StdInc.h"

CAntiDebugging::CAntiDebugging(void* (*DetectionCallback)(eDebugDetectionFlags, std::string))
{
    m_DetectionCallback = DetectionCallback;
}

void CAntiDebugging::StartPulse()
{
    CAtomicThread::Create(&CAntiDebugging::StaticPulse, this);
}

void CAntiDebugging::_IsHardwareDebuggerPresent()
{
    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE)
    {
        SharedUtil::AddDebugLog("[ANTIDEBUGGING] CreateToolhelp32Snapshot (of threads) failed: %d", GetLastError());
        return;
    }

    DWORD dwCurrentProcessID = GetCurrentProcessId();

    if (Thread32First(hThreadSnap, &te32))
    {
        do
        {
            if (te32.th32OwnerProcessID == dwCurrentProcessID)
            {
                HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);
                if (hThread)
                {
                    CONTEXT ctx;
                    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                    if (GetThreadContext(hThread, &ctx))
                    {
                        if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3 || ctx.Dr6 || ctx.Dr7)
                        {
                            SharedUtil::AddDebugLog("[ANTIDEBUGGING] Hardware breakpoint detected in thread ID: %d", te32.th32ThreadID);
                            m_DetectionCallback(eDebugDetectionFlags::DEBUG_HARDWARE_REGISTERS, "Debugger Detected");
                        }
                    }
                    else
                    {
                        SharedUtil::AddDebugLog("[ANTIDEBUGGING] GetThreadContext failed for thread ID %d: %d", te32.th32ThreadID, GetLastError());
                    }
                    CloseHandle(hThread);
                }
                else
                {
                    SharedUtil::AddDebugLog("[ANTIDEBUGGING] OpenThread failed for thread ID %d: %d", te32.th32ThreadID, GetLastError());
                }
            }
        } while (Thread32Next(hThreadSnap, &te32));
    }
    else
    {
        SharedUtil::AddDebugLog("[ANTIDEBUGGING] Thread32First failed: %d", GetLastError());
    }
}

bool CAntiDebugging::PreventWindowsDebuggers()
{
    HMODULE hNtDll = GetModuleHandle("ntdll.dll");

    if (!hNtDll)
    {
        SharedUtil::AddDebugLog("[ANTIDEBUGGING] GetModuleHandle for ntdll.dll failed: %d", GetLastError());
        return false;
    }

    DWORD dwOldProtect;

    UINT64 DbgBreakpoint_Address = (UINT64)GetProcAddress(hNtDll, "DbgBreakPoint");
    UINT64 DbgUiRemoteBreakin_Address = (UINT64)GetProcAddress(hNtDll, "DbgUiRemoteBreakin");

    if (DbgBreakpoint_Address)
    {
        if (VirtualProtect((LPVOID)DbgBreakpoint_Address, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
        {
            __try
            {
                *((BYTE*)DbgBreakpoint_Address) = 0xC3;            // RET instruction
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Exception occurred while patching DbgBreakPoint: %d", GetExceptionCode());
                return false;
            }

            VirtualProtect((LPVOID)DbgBreakpoint_Address, 1, dwOldProtect, &dwOldProtect);
            SharedUtil::AddDebugLog("[ANTIDEBUGGING] Patched DbgBreakPoint at address: 0x%p", DbgBreakpoint_Address);
        }
        else
        {
            SharedUtil::AddDebugLog("[ANTIDEBUGGING] VirtualProtect failed for DbgBreakPoint: %d", GetLastError());
            return false;
        }

        if (DbgUiRemoteBreakin_Address)
        {
            if (VirtualProtect((LPVOID)DbgUiRemoteBreakin_Address, 1, PAGE_EXECUTE_READWRITE, &dwOldProtect))
            {
                __try
                {
                    *((BYTE*)DbgUiRemoteBreakin_Address) = 0xC3;    // RET instruction
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    SharedUtil::AddDebugLog("[ANTIDEBUGGING] Exception occurred while patching DbgUiRemoteBreakin: %d", GetExceptionCode());
                    return false;
                }
                VirtualProtect((LPVOID)DbgUiRemoteBreakin_Address, 1, dwOldProtect, &dwOldProtect);
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Patched DbgUiRemoteBreakin at address: 0x%p", (LPVOID)DbgUiRemoteBreakin_Address);
            }
            else
            {
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] VirtualProtect failed for DbgUiRemoteBreakin: %d", GetLastError());
                return false;
            }
        }
        else
        {
            SharedUtil::AddDebugLog("[ANTIDEBUGGING] GetProcAddress failed for DbgUiRemoteBreakin: %d", GetLastError());
            return false;
        }
    }
    else
    {
        SharedUtil::AddDebugLog("[ANTIDEBUGGING] GetProcAddress failed for DbgBreakPoint: %d", GetLastError());
        return false;
    }
    
    return true;
}

eDebugDetectionFlags CAntiDebugging::_IsDebuggerPresent()
{
    return (IsDebuggerPresent() ? eDebugDetectionFlags::DEBUG_WINAPI_DEBUGGER : eDebugDetectionFlags::NONE);
}

eDebugDetectionFlags CAntiDebugging::_IsKernelDebuggerPresent()
{
    typedef long NTSTATUS;
    HANDLE       hProcess = GetCurrentProcess();

    typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION
    {
        bool DebuggerEnabled;
        bool DebuggerNotPresent;
    } SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

    enum SYSTEM_INFORMATION_CLASS
    {
        SystemKernelDebuggerInformation = 35
    };
    typedef NTSTATUS(__stdcall * NT_QUERY_SYSTEM_INFORMATION)(IN SYSTEM_INFORMATION_CLASS SystemInformationClass, IN OUT PVOID SystemInformation,
                                                              IN ULONG SystemInformationLength, OUT PULONG ReturnLength);
    NT_QUERY_SYSTEM_INFORMATION        NtQuerySystemInformation;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION Info;

    HMODULE hModule = GetModuleHandleA("ntdll.dll");

    if (hModule == NULL)
    {
        SharedUtil::AddDebugLog("[ANTIDEBUGGING] Error fetching module ntdll.dll @ _IsKernelDebuggerPresent: %d", GetLastError());
        return EXECUTION_ERROR;
    }

    NtQuerySystemInformation = (NT_QUERY_SYSTEM_INFORMATION)GetProcAddress(hModule, "NtQuerySystemInformation");
    if (NtQuerySystemInformation == NULL)
        return EXECUTION_ERROR;

    if (NtQuerySystemInformation(SystemKernelDebuggerInformation, &Info, sizeof(Info), NULL))
    {
        if (Info.DebuggerEnabled || !Info.DebuggerNotPresent)
        {
            return DEBUG_KERNEL_DEBUGGER;
        }
    }
    else
        return EXECUTION_ERROR;

    return NONE;
}

eDebugDetectionFlags CAntiDebugging::_IsKernelDebuggerPresent_SharedKData()
{
    _KUSER_SHARED_DATA* sharedData = USER_SHARED_DATA;
    bool                bDebuggerEnabled = false;

    if (sharedData != nullptr && sharedData->KdDebuggerEnabled)
    {
        bDebuggerEnabled = true;
    }

    return bDebuggerEnabled ? DEBUG_KERNEL_DEBUGGER : NONE;
}

/*
    _IsDebuggerPresent_HeapFlags - checks heap flags in the PEB, certain combination can indicate a debugger is present
*/
eDebugDetectionFlags CAntiDebugging::_IsDebuggerPresent_HeapFlags()
{
#ifdef _M_IX86
    DWORD_PTR pPeb64 = (DWORD_PTR)__readfsdword(0x30);
#else
    DWORD_PTR pPeb64 = (DWORD_PTR)__readgsqword(0x60);
#endif

    if (pPeb64)
    {
        PVOID  ptrHeap = (PVOID) * (PDWORD_PTR)((PBYTE)pPeb64 + 0x30);
        PDWORD heapForceFlagsPtr = (PDWORD)((PBYTE)ptrHeap + 0x74);

        if (ptrHeap && heapForceFlagsPtr)
        {
            if (*heapForceFlagsPtr >= 0x40000060)
            {
                return DEBUG_HEAP_FLAG;
            }
        }
    }

    return NONE;
}

/*
  _IsDebuggerPresent_CloseHandle - calls CloseHandle with an invalid handle, if an exception is thrown then a debugger is present
*/
eDebugDetectionFlags CAntiDebugging::_IsDebuggerPresent_CloseHandle()
{
#ifndef _DEBUG
    __try
    {
        CloseHandle((HANDLE)1);
    }
    __except (EXCEPTION_INVALID_HANDLE == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return DEBUG_CLOSEHANDLE;
    }
#endif
    return NONE;
}

eDebugDetectionFlags CAntiDebugging::_IsDebuggerPresent_RemoteDebugger()
{
    BOOL bDebugged = FALSE;

    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &bDebugged))
    {
        if (bDebugged)
        {
            return DEBUG_REMOTE_DEBUGGER;
        }
    }

    return NONE;
}

/*
    _IsDebuggerPresent_VEH - Checks if vehdebug-x86_64.dll is loaded and exporting InitiallizeVEH. If so, the first byte of this routine is patched and the
   module's internal name is changed to STOP_CHEATING returns true if CE's VEH debugger is found, but this won't stop home-rolled VEH debuggers via APC
   injection
*/
eDebugDetectionFlags CAntiDebugging::_IsDebuggerPresent_VEH()
{
    bool bFound = false;

    HMODULE veh_debugger = GetModuleHandleA("vehdebug-x86_64.dll");            // if someone renames this dll we'll still stop them from debugging since our TLS
                                                                               // callback patches over first byte of new thread funcs

    if (veh_debugger != NULL)
    {
        uintptr_t veh_addr = (uintptr_t)GetProcAddress(veh_debugger, "InitializeVEH");            // check for named exports of cheat engine's VEH debugger

        if (veh_addr > 0)
        {
            bFound = true;

            DWORD dwOldProt = 0;

            if (!VirtualProtect((void*)veh_addr, 1, PAGE_EXECUTE_READWRITE, &dwOldProt))
            {
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] VirtualProtect failed @ _IsDebuggerPresent_VEH");
                return DEBUG_VEH_DEBUGGER;            // return true since we found the routine, even though we can't patch over it. if virtualprotect fails,
                                                      // the program will probably crash if trying to patch it
            }

            memcpy((void*)veh_addr, "\xC3",
                   sizeof(BYTE));            // patch first byte of `InitializeVEH` with a ret, stops call to InitializeVEH from succeeding.

            if (!VirtualProtect((void*)veh_addr, 1, dwOldProt, &dwOldProt))            // change back to old prot's
            {
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] VirtualProtect failed @ _IsDebuggerPresent_VEH");
            }
        }
    }

    return (bFound ? DEBUG_VEH_DEBUGGER : NONE);
}

/*
     _IsDebuggerPresent_PEB - checks the PEB for the BeingDebugged flag
     returns `true` if byte is set to 1, indicating a debugger is present
*/
eDebugDetectionFlags CAntiDebugging::_IsDebuggerPresent_PEB()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll)
    {
        SharedUtil::AddDebugLog("[AntiDebug] ntdll.dll not found; can't query PEB.");
        return eDebugDetectionFlags::EXECUTION_ERROR;
    }

    auto NtQueryInformationProcess = (NtQueryInformationProcess_t)GetProcAddress(hNtdll, "NtQueryInformationProcess");
    if (!NtQueryInformationProcess)
    {
        SharedUtil::AddDebugLog("[AntiDebug] NtQueryInformationProcess not found in ntdll.dll.");
        return eDebugDetectionFlags::EXECUTION_ERROR;
    }

    PROCESS_BASIC_INFORMATION_INTERNAL pbi = {0};
    ULONG                              returnedLength = 0;
    NTSTATUS status = NtQueryInformationProcess(GetCurrentProcess(), 0 /* ProcessBasicInformation */, &pbi, sizeof(pbi), &returnedLength);

    if (status < 0)
    {
        SharedUtil::AddDebugLog("[AntiDebug] NtQueryInformationProcess failed (status=0x%08X).", status);
        return eDebugDetectionFlags::EXECUTION_ERROR;
    }

    // pbi.PebBaseAddress points to the PEB in our process.
    volatile BYTE* pebBeingDebugged = nullptr;
    if (pbi.PebBaseAddress == nullptr)
    {
        SharedUtil::AddDebugLog("[AntiDebug] PEB base address was NULL.");
        return eDebugDetectionFlags::EXECUTION_ERROR;
    }

    // The BeingDebugged byte is at offset 2 in the PEB on supported Windows versions.
    // We'll read it via direct memory access (we're in the same process).
    // To be defensive, copy into a local variable.
    bool detected = false;
    __try
    {
        // safe direct access (we are in the same process)
        BYTE beingDebugged = *(volatile BYTE*)((BYTE*)pbi.PebBaseAddress + 2);
        detected = (beingDebugged != 0);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        SharedUtil::AddDebugLog("[AntiDebug] Exception while reading PEB->BeingDebugged.");
        return eDebugDetectionFlags::EXECUTION_ERROR;
    }

    if (detected)
    {
        SharedUtil::AddDebugLog("[AntiDebug] Debugger detected via PEB->BeingDebugged (value != 0).");
        return DEBUG_PEB;
    }

    return eDebugDetectionFlags::NONE;
}

/*
    _IsDebuggerPresent_DebugPort - calls NtQueryInformationProcess with PROCESS_INFORMATION_CLASS 0x07 to check for debuggers
*/
eDebugDetectionFlags CAntiDebugging::_IsDebuggerPresent_DebugPort()
{
    typedef NTSTATUS(NTAPI * TNtQueryInformationProcess)(IN HANDLE ProcessHandle, IN PROCESS_INFORMATION_CLASS ProcessInformationClass,
                                                         OUT PVOID ProcessInformation, IN ULONG ProcessInformationLength, OUT PULONG ReturnLength);

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");

    if (hNtdll)
    {
        auto pfnNtQueryInformationProcess = (TNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");

        if (pfnNtQueryInformationProcess)
        {
            const PROCESS_INFORMATION_CLASS ProcessDebugPort = (PROCESS_INFORMATION_CLASS)7;
            DWORD                           dwProcessDebugPort, dwReturned;
            NTSTATUS status = pfnNtQueryInformationProcess(GetCurrentProcess(), ProcessDebugPort, &dwProcessDebugPort, sizeof(DWORD), &dwReturned);

            if (NT_SUCCESS(status) && (dwProcessDebugPort == -1))
            {
                return DEBUG_DEBUG_PORT;
            }
        }
        else
        {
            SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to fetch NtQueryInformationProcess address @ _IsDebuggerPresent_DebugPort ");
            return EXECUTION_ERROR;
        }
    }
    else
    {
        SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to fetch ntdll.dll address @ _IsDebuggerPresent_DebugPort ");
        return EXECUTION_ERROR;
    }

    return NONE;
}

/*
    _IsDebuggerPresent_ProcessDebugFlags - calls NtQueryInformationProcess with PROCESS_INFORMATION_CLASS 0x1F to check for debuggers
*/
eDebugDetectionFlags CAntiDebugging::_IsDebuggerPresent_ProcessDebugFlags()
{
    typedef NTSTATUS(NTAPI * TNtQueryInformationProcess)(IN HANDLE ProcessHandle, IN PROCESS_INFORMATION_CLASS ProcessInformationClass,
                                                         OUT PVOID ProcessInformation, IN ULONG ProcessInformationLength, OUT PULONG ReturnLength);

    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");

    if (hNtdll)
    {
        auto pfnNtQueryInformationProcess = (TNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");

        if (pfnNtQueryInformationProcess)
        {
            PROCESS_INFORMATION_CLASS pic = (PROCESS_INFORMATION_CLASS)0x1F;
            DWORD                     dwProcessDebugFlags, dwReturned;
            NTSTATUS                  status = pfnNtQueryInformationProcess(GetCurrentProcess(), pic, &dwProcessDebugFlags, sizeof(DWORD), &dwReturned);

            if (NT_SUCCESS(status) && (dwProcessDebugFlags == 0))
            {
                return DEBUG_PROCESS_DEBUG_FLAGS;
            }
        }
    }
    else
    {
        SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to fetch ntdll.dll address @ _IsDebuggerPresent_ProcessDebugFlags ");
        return EXECUTION_ERROR;
    }

    return NONE;
}

/*
    _ExitCommonDebuggers - create remote thread on `ExitProcess` in any common debugger processes
    This can of course be bypassed with a simple process name change, preferrably we would use a combination of artifacts to find these processes
*/
eDebugDetectionFlags CAntiDebugging::_ExitCommonDebuggers(std::string* strReason)
{
    bool triedEndDebugger = false;

    for (const std::string& debugger : m_vCommonDebuggerProcesses)
    {
        std::list<DWORD> pids = Utils::GetProcessIdsByName(debugger);

        for (const auto pid : pids)
        {
            triedEndDebugger = true;
            *strReason = "Debugger Process Detected: " + debugger;

            uintptr_t K32Base = (uintptr_t)GetModuleHandleW(L"kernel32.dll");

            if (K32Base == NULL)
            {
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to fetch kernel32.dll address @ _ExitCommonDebuggers ");
                return EXECUTION_ERROR;
            }

            uintptr_t ExitProcessAddr = (uintptr_t)GetProcAddress((HMODULE)K32Base, "ExitProcess");

            if (ExitProcessAddr == NULL)
            {
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to fetch ExitProcess address @ _ExitCommonDebuggers ");
                return EXECUTION_ERROR;
            }

            uintptr_t ExitProcessOffset = ExitProcessAddr - K32Base;

            HANDLE remoteProcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

            if (remoteProcHandle)
            {
                uintptr_t FunctionAddr_ExitProcess = (uintptr_t)Utils::GetRemoteModuleBaseAddress(pid, "kernel32.dll") + ExitProcessOffset;
                HANDLE    RemoteThread = CreateRemoteThread(remoteProcHandle, 0, 0, (LPTHREAD_START_ROUTINE)FunctionAddr_ExitProcess, 0, 0, 0);
                CloseHandle(remoteProcHandle);
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Attempting to terminate debugger process %s with pid %d @ _ExitCommonDebuggers", debugger.c_str(),
                                        pid);
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Created remote thread at %llX address", FunctionAddr_ExitProcess);

                return DEBUG_KNOWN_DEBUGGER_PROCESS;
            }
            else
            {
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to open process handle for pid %d @ _ExitCommonDebuggers", pid);
            }
        }
    }

    return (triedEndDebugger ? DEBUG_KNOWN_DEBUGGER_PROCESS : NONE);
}

eDebugDetectionFlags CAntiDebugging::_ExitCommonDebuggerWindows(std::string* strReason)
{
    bool triedEndDebugger = false;

    HWND hwnd = GetTopWindow(NULL);
    while (hwnd)
    {
        char windowTitle[256];
        GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

        if (IsWindowVisible(hwnd) && strlen(windowTitle) > 0)
        {
            std::string title = windowTitle;

            for (const std::string& dbgTitle : vCommonDebuggerWindows)
            {
                if (title.find(dbgTitle) != std::string::npos)
                {
                    DWORD pid = 0;
                    GetWindowThreadProcessId(hwnd, &pid);

                    if (pid != 0)
                    {
                        triedEndDebugger = true;
                        *strReason = "Debugger Window Detected: " + title;


                        uintptr_t K32Base = (uintptr_t)GetModuleHandleW(L"kernel32.dll");
                        if (!K32Base)
                        {
                            SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to fetch kernel32.dll address @ _ExitCommonDebuggerWindows");
                            return EXECUTION_ERROR;
                        }

                        uintptr_t ExitProcessAddr = (uintptr_t)GetProcAddress((HMODULE)K32Base, "ExitProcess");
                        if (!ExitProcessAddr)
                        {
                            SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to fetch ExitProcess address @ _ExitCommonDebuggerWindows");
                            return EXECUTION_ERROR;
                        }

                        uintptr_t ExitProcessOffset = ExitProcessAddr - K32Base;

                        HANDLE remoteProcHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
                        if (remoteProcHandle)
                        {
                            uintptr_t FunctionAddr_ExitProcess = (uintptr_t)Utils::GetRemoteModuleBaseAddress(pid, "kernel32.dll") + ExitProcessOffset;

                            HANDLE RemoteThread = CreateRemoteThread(remoteProcHandle, 0, 0, (LPTHREAD_START_ROUTINE)FunctionAddr_ExitProcess, 0, 0, 0);

                            CloseHandle(remoteProcHandle);

                            SharedUtil::AddDebugLog("[ANTIDEBUGGING] Attempting to terminate debugger window '%s' (pid %d)", title.c_str(), pid);
                            SharedUtil::AddDebugLog("[ANTIDEBUGGING] Created remote thread at %llX address", FunctionAddr_ExitProcess);
                        }
                        else
                        {
                            SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to open process handle for pid %d @ _ExitCommonDebuggerWindows", pid);
                        }
                    }
                }
            }
        }

        hwnd = GetNextWindow(hwnd, GW_HWNDNEXT);
    }

    return (triedEndDebugger ? DEBUG_KNOWN_DEBUGGER_WINDOW : NONE);
}


void CAntiDebugging::DoPulse()
{
    while (true)
    {
        if (m_DetectionCallback == nullptr)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        std::string strReason;

        _IsHardwareDebuggerPresent();
        eDebugDetectionFlags dbgFlag = _IsDebuggerPresent();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Debugging behaviour detected");
        dbgFlag = _IsDebuggerPresent_HeapFlags();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Memory debugging activity detected");
        dbgFlag = _IsDebuggerPresent_CloseHandle();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Debugging activity detected");
        dbgFlag = _IsDebuggerPresent_RemoteDebugger();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Remote debugger detected");
        dbgFlag = _IsDebuggerPresent_VEH();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Debugging behaviour detected");
        dbgFlag = _IsDebuggerPresent_PEB();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Debugging behaviour detected");
        dbgFlag = _IsDebuggerPresent_DebugPort();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Debugging behaviour detected");
        dbgFlag = _IsDebuggerPresent_ProcessDebugFlags();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Debugger Detected");
        dbgFlag = _IsKernelDebuggerPresent();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Kernel Debugger Detected");
        dbgFlag = _IsKernelDebuggerPresent_SharedKData();
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, "Kernel debugging activity");
        dbgFlag = _ExitCommonDebuggers(&strReason);
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, strReason);
        dbgFlag = _ExitCommonDebuggerWindows(&strReason);
        if (dbgFlag != NONE)
            m_DetectionCallback(dbgFlag, strReason);
        Sleep(2000);            // pulse every 2 seconds
    }
}