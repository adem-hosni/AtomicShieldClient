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
    m_lpStartAddress = lpStartAddress;
    m_lpParameter = lpParameter;
}

CAtomicThread::~CAtomicThread()
{
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

    if (!m_hThread)
    {
        SharedUtil::AddDebugLog("Failed to create thread handle on 0x%x Error code: 0x%x", (DWORD64)m_lpStartAddress, GetLastError());
        return false;
    }
    return true;

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