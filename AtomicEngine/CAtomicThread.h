#pragma once
#include "StdInc.h"

typedef void*(NTAPI* PFNNTCREATETHREADEX)(PHANDLE hThread, ACCESS_MASK DesiredAccess, PVOID ObjectAttributes, HANDLE ProcessHandle,
                                             PVOID lpStartAddress, PVOID lpParameter, ULONG Flags, SIZE_T StackZeroBits,
                                             SIZE_T SizeOfStackCommit, SIZE_T SizeOfStackReserve, PVOID lpBytesBuffer);

typedef void*(NTAPI* PFNNTSETINFORMATIONTHREAD)(HANDLE ThreadHandle, void* ThreadInformationClass, PVOID ThreadInformation,
                                                   ULONG ThreadInformationLength);

class CAtomicThread
{
public:
    CAtomicThread(PVOID lpStartAddress, PVOID lpParameter = nullptr);
    ~CAtomicThread();

    bool Create();
    bool Terminate() { return TerminateThread(m_hThread, NULL); }

    static CAtomicThread* Create(LPVOID lpStartAddress, LPVOID lpParameter = nullptr);
    HANDLE GetHandle() { return m_hThread; }

private:
    PVOID m_lpStartAddress;
    PVOID m_lpParameter;

    PFNNTCREATETHREADEX m_NtCreateThreadEx;
    PFNNTSETINFORMATIONTHREAD m_NtSetInformationThread;
    HANDLE m_hThread;
};
