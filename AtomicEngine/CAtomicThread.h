#pragma once
#include "StdInc.h"
#include "KernelCalls.hpp"

typedef enum _LocalTHREADINFOCLASS
{
    ThreadBasicInformation = 0
} LocalTHREADINFOCLASS;

typedef void*(NTAPI* PFNNTCREATETHREADEX)(PHANDLE hThread, ACCESS_MASK DesiredAccess, PVOID ObjectAttributes, HANDLE ProcessHandle, PVOID lpStartAddress,
                                          PVOID lpParameter, ULONG Flags, SIZE_T StackZeroBits, SIZE_T SizeOfStackCommit, SIZE_T SizeOfStackReserve,
                                          PVOID lpBytesBuffer);

typedef void*(NTAPI* PFNNTSETINFORMATIONTHREAD)(HANDLE ThreadHandle, void* ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);

typedef NTSTATUS(NTAPI* PFNNTQUERYINFORMATIONTHREAD)(HANDLE ThreadHandle, LocalTHREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation,
                                                     ULONG ThreadInformationLength, PULONG ReturnLength);

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS              ExitStatus;
    PVOID                 TebBaseAddress;
    KernelCalls_CLIENT_ID ClientId;
    ULONG_PTR             AffinityMask;
    LONG                  Priority;
    LONG                  BasePriority;
    ULONG                 SuspendCount;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

class CAtomicThread
{
public:
    CAtomicThread(PVOID lpStartAddress, PVOID lpParameter = nullptr);
    ~CAtomicThread();

    bool Create();
    bool Terminate() { return TerminateThread(m_hThread, NULL); }

    static CAtomicThread* Create(LPVOID lpStartAddress, LPVOID lpParameter = nullptr);
    HANDLE                GetHandle() { return m_hThread; }

    bool IsHandleValid();
    bool IsTerminated();
    bool IsSuspended();

private:
    PVOID m_lpStartAddress;
    PVOID m_lpParameter;

    PFNNTCREATETHREADEX       m_NtCreateThreadEx;
    PFNNTSETINFORMATIONTHREAD m_NtSetInformationThread;
    HANDLE                    m_hThread;
};
