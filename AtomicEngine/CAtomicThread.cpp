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
    m_hThread = reinterpret_cast<HANDLE>(_beginthread((_beginthread_proc_type)m_lpStartAddress, NULL, m_lpParameter));
    return true;
}

bool CAtomicThread::Terminate()
{
    bool bSuccess = false;
    if (m_hThread)
    {
        bSuccess = TerminateThread(m_hThread, 0);
        CloseHandle(m_hThread);
        m_hThread = nullptr;
    }

    return bSuccess;
}

CAtomicThread* CAtomicThread::Create(LPVOID lpStartAddress, LPVOID lpParameter)
{
    CAtomicThread* pAtomicThread = new CAtomicThread(lpStartAddress, lpParameter);
    pAtomicThread->Create();

    return pAtomicThread;
}