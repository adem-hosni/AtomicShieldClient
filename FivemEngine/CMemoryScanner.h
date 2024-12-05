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
    bool IsAttached() { return m_hProcess != NULL; }

    static void            ScanMemoryRegion(HANDLE hProcess, LPVOID start, LPVOID end, size_t bufferSize);
    void                   ScanStrings(std::map<std::string, std::vector<std::string>> Signatures);
    std::vector<uintptr_t> GetVMAddresses() { return m_vAddresses; };

    void                     AddSignatures(jsoncons::json Signatures);
    std::vector<std::string> GetDetectedSignatures() { return m_vFoundSignatures; }

    unsigned int GetLatestScanResult() { return m_uiLatestScanResult; }
    void         UpdateLatestScanResult(unsigned int uiScanResult) { m_uiLatestScanResult = uiScanResult; }

private:
    std::vector<uintptr_t>   m_vAddresses;
    MEMORY_BASIC_INFORMATION m_MBI;

    unsigned int m_uiLatestScanResult;

    std::vector<std::string> m_vFoundSignatures;

    HANDLE      m_hProcess;
    DWORD       m_dwProcessID;
    SYSTEM_INFO m_SystemInfo;
    char*       m_szCurrentMemoryPage = 0;
};

extern CMemoryScanner* g_pMemoryScanner;
