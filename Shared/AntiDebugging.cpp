#include "SharedUtil.h"
#include "CAntiDebugging.h"

CAntiDebugging::CAntiDebugging(void* (*DetectionCallback)(eDebugDetectionFlags))
{
    m_DetectionCallback = DetectionCallback;
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
                            m_DetectionCallback(eDebugDetectionFlags::DEBUG_HARDWARE_REGISTERS);
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
#ifdef _M_IX86
    MYPEB* _PEB = (MYPEB*)__readfsdword(0x30);
#else
    MYPEB* _PEB = (MYPEB*)__readgsqword(0x60);
#endif

    bool bDebuggerPresent = false;

    if (_PEB != nullptr && _PEB->BeingDebugged)
    {
        bDebuggerPresent = true;
    }

    return (bDebuggerPresent ? DEBUG_PEB : NONE);
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
eDebugDetectionFlags CAntiDebugging::_ExitCommonDebuggers()
{
    bool triedEndDebugger = false;

    for (const std::wstring& debugger : this->CommonDebuggerProcesses)
    {
        std::list<DWORD> pids = Process::GetProcessIdsByName(debugger);

        for (const auto pid : pids)
        {
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
                uintptr_t FunctionAddr_ExitProcess = (uintptr_t)Process::GetRemoteModuleBaseAddress(pid, L"kernel32.dll") + ExitProcessOffset;
                HANDLE    RemoteThread = CreateRemoteThread(remoteProcHandle, 0, 0, (LPTHREAD_START_ROUTINE)FunctionAddr_ExitProcess, 0, 0, 0);
                triedEndDebugger = true;
                CloseHandle(remoteProcHandle);
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Created remote thread at %llX address", FunctionAddr_ExitProcess);
            }
            else
            {
                SharedUtil::AddDebugLog("[ANTIDEBUGGING] Failed to open process handle for pid %d @ _ExitCommonDebuggers", pid);
            }
        }
    }

    return (triedEndDebugger ? DEBUG_KNOWN_DEBUGGER_PROCESS : NONE);
}
