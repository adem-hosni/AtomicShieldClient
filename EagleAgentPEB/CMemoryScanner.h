#pragma once
#include "StdInc.h"
#include <jsoncons/json.hpp>
#include <Windows.h>
#include <winternl.h>
#include <vector>
#include <string>
#include <Psapi.h>
#include <iostream>
#pragma comment(lib, "ntdll.lib")


typedef enum _MEMORY_INFORMATION_CLASS
{
    MemoryBasicInformation
} MEMORY_INFORMATION_CLASS;

extern "C" NTSYSCALLAPI NTSTATUS ZwReadVirtualMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);

extern "C" NTSYSCALLAPI NTSTATUS ZwWriteVirtualMemory(HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten);

extern "C" NTSYSCALLAPI NTSTATUS NtQueryVirtualMemory(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass,
                                                      PVOID MemoryInformation, SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);

class CMemoryScanner
{
public:
    CMemoryScanner();
    ~CMemoryScanner();

    void Attach(DWORD dwProcessID);

    void                   ScanStrings();
    std::vector<uintptr_t> GetVMAddresses() { return m_vAddresses; };

    void                                AddSignatures(jsoncons::json Signatures);
    std::vector<jsoncons::json> GetSignatures() { return m_vSignatures; }

private:
    void debug(std::string printthatshit);

private:
    std::vector<uintptr_t>   m_vAddresses;
    MEMORY_BASIC_INFORMATION m_MBI;

    std::vector<std::string>    m_vFoundSignatures;
    std::vector<jsoncons::json> m_vSignatures;

    HANDLE      m_hProcess;
    SYSTEM_INFO m_SystemInfo;
    char*       m_szCurrentMemoryPage = 0;
};

inline CMemoryScanner* g_pSignatureScanner = NULL;
