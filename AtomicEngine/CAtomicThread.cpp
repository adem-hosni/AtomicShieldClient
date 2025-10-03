#include "StdInc.h"
#include <winver.h>
#include <Psapi.h>
#include <algorithm>
#include <random>
#include <TlHelp32.h>

#define THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE 0x40
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER    0x00000004

CAtomicThread::CAtomicThread(PVOID lpStartAddress, PVOID lpParameter)
{
    m_hThread = INVALID_HANDLE_VALUE;

    HMODULE hModule = LoadLibrary("ntdll.dll");
    m_NtCreateThreadEx = (PFNNTCREATETHREADEX)GetProcAddress(hModule, "NtCreateThreadEx");
    m_NtSetInformationThread = (PFNNTSETINFORMATIONTHREAD)GetProcAddress(hModule, "NtSetInformationThread");
    m_NtQueryInformationThread = (PFNNTQUERYINFORMATIONTHREAD)GetProcAddress(hModule, "NtQueryInformationThread");
    m_NtTerminateThread = (PFNNTTERMINATETHREAD)GetProcAddress(hModule, "NtTerminateThread");
    m_lpStartAddress = lpStartAddress;
    m_lpParameter = lpParameter;
    m_iThreadID = NULL;
}

CAtomicThread::~CAtomicThread()
{
    if (m_hThread && m_hThread != INVALID_HANDLE_VALUE)
    {
        Terminate();
    }
    m_iThreadID = NULL;
}

bool CAtomicThread::Create()
{
    if (!m_NtCreateThreadEx || !m_NtSetInformationThread)
    {
        SharedUtil::AddDebugLog("Unable to create thread on 0x%x: m_NtCreateThreadEx got 0x%x m_NtSetInformationThread: 0x%x", (DWORD64)m_lpStartAddress,
                                (DWORD64)m_NtCreateThreadEx, (DWORD64)m_NtSetInformationThread);
        return false;
    }

    m_NtCreateThreadEx(&m_hThread, MAXIMUM_ALLOWED, nullptr, GetCurrentProcess(), m_lpStartAddress, m_lpParameter,
                       THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE | THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER, 0, 0, 0, nullptr);
    SharedUtil::SetPrivilege(SE_DEBUG_NAME);

    if (!m_hThread || m_hThread == INVALID_HANDLE_VALUE)
    {
        SharedUtil::AddDebugLog("Failed to create thread handle on 0x%x Error code: 0x%x", (DWORD64)m_lpStartAddress, GetLastError());
        return false;
    }
    m_iThreadID = GetThreadId(m_hThread);

    SharedUtil::AddDebugLog("Thread created at 0x%x with TID %d", (DWORD64)m_lpStartAddress, m_iThreadID);

    ULONG    ulEnable = true;
    NTSTATUS NTThreadBreakOnTermination = (NTSTATUS)m_NtSetInformationThread(m_hThread, (void*)18, &ulEnable, sizeof(ulEnable));
    if (!NT_SUCCESS(NTThreadBreakOnTermination))
    {
        SharedUtil::AddDebugLog("Unable to set thread protection! status: 0x%llx last error: 0x%llx", NTThreadBreakOnTermination, GetLastError());
    }

    g_pAtomicAntiCheat->GetAtomicThreads().push_back(this);
    return true;
}

CAtomicThread* CAtomicThread::Create(LPVOID lpStartAddress, LPVOID lpParameter)
{
    CAtomicThread* pAtomicThread = new CAtomicThread(lpStartAddress, lpParameter);
    pAtomicThread->Create();

    return pAtomicThread;
}

bool CAtomicThread::Terminate()
{
    if (!m_NtTerminateThread || !m_hThread || m_hThread == INVALID_HANDLE_VALUE)
        return false;
    NTSTATUS status = m_NtTerminateThread(m_hThread, 0);
    if (!NT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to terminate thread! status: 0x%llx last error: 0x%llx", (unsigned long long)status, GetLastError());
        return false;
    }
    else
        SharedUtil::AddDebugLog("Thread at 0x%x terminated successfuly", m_lpStartAddress);
    CloseHandle(m_hThread);
    m_hThread = NULL;
    return true;
}

bool CAtomicThread::IsHandleValid()
{
    if (!m_hThread || m_hThread == INVALID_HANDLE_VALUE)
        return false;

    DWORD  dup = 0;
    HANDLE hTemp = NULL;
    BOOL   bOk = DuplicateHandle(GetCurrentProcess(), m_hThread, GetCurrentProcess(), &hTemp, 0, FALSE, DUPLICATE_SAME_ACCESS);
    if (bOk)
    {
        CloseHandle(hTemp);
        return true;
    }
    return false;
}

bool CAtomicThread::IsTerminated()
{
    if (!m_hThread)
        return true;
    DWORD dwResult = WaitForSingleObject(m_hThread, 0);
    return (dwResult == WAIT_OBJECT_0);
}

bool CAtomicThread::IsSuspended()
{
    if (!m_hThread || m_hThread == INVALID_HANDLE_VALUE)
        return false;

    DWORD exitCode = 0;
    if (GetExitCodeThread(m_hThread, &exitCode) && exitCode != STILL_ACTIVE)
        return false;

    if (m_NtQueryInformationThread)
    {
        ULONG suspendCount = 0;
        ULONG retLen = 0;

        NTSTATUS status = m_NtQueryInformationThread(m_hThread,
                                                     0x11,            // ThreadSuspendCount
                                                     &suspendCount,
                                                     sizeof(suspendCount),            // MUST be 4
                                                     &retLen);

        if (NT_SUCCESS(status))
        {
            bool suspended = (suspendCount > 0);
            SharedUtil::AddDebugLog("[IsSuspended] SuspendCount=%u => suspended=%d", suspendCount, suspended ? 1 : 0);
            return suspended;
        }
        else
        {
            SharedUtil::AddDebugLog("[IsSuspended] NtQueryInformationThread(ThreadSuspendCount) failed (0x%llx 0x%x).", (unsigned long long)status,
                                    GetLastError());
        }
    }

    SharedUtil::AddDebugLog("[IsSuspended] Could not determine state; assuming not suspended.");
    return false;
}
