#include <fstream>
#include "StdInc.h"
#include "KernelCalls.hpp"

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
}

void CHeuristicGuard::Initialize()
{
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::unordered_set<std::string>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            m_Signatures.insert(Signature);
        }
    }

    // SpawnScanProcess();
}

void CHeuristicGuard::ScanForString()
{
    std::wstring      memoryString = L"Dear ImGui Demo";
    std::wstring_view wstr(memoryString.begin(), memoryString.end());

    const DWORD  currentProcessId = GetCurrentProcessId();
    const HANDLE currentProcess = GetCurrentProcess();

    DWORD targetProcessId = GetCurrentProcessId();

    NTSTATUS                      status;
    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
    KernelCalls_CLIENT_ID         clientId{};
    HANDLE                        processHandle = GetCurrentProcess();

    RtlSecureZeroMemory(&objAttr, sizeof(KernelCalls_OBJECT_ATTRIBUTES));
    objAttr.Length = sizeof(KernelCalls_OBJECT_ATTRIBUTES);
    RtlSecureZeroMemory(&clientId, sizeof(KernelCalls_CLIENT_ID));
    clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(targetProcessId));

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    MEMORY_BASIC_INFORMATION memoryInfo{};
    bool                     found = false;

    for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
         addr = static_cast<LPBYTE>(memoryInfo.BaseAddress) + memoryInfo.RegionSize)
    {
        PVOID  baseAddress = addr;
        SIZE_T regionSize = sizeof(memoryInfo);
        SIZE_T returnLength;

        status = SysNtQueryVirtualMemory(processHandle, baseAddress, MemoryBasicInformation, &memoryInfo, regionSize, &returnLength);
        if (!NT_SUCCESS(status) || memoryInfo.State != MEM_COMMIT || memoryInfo.Protect & PAGE_NOACCESS)
            continue;

        SIZE_T allocationSize = memoryInfo.RegionSize + wstr.size() * sizeof(wchar_t) - 1;            // Extra space for overlap
        PVOID  buffer = nullptr;
        status = SysNtAllocateVirtualMemory(currentProcess, &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!NT_SUCCESS(status))
            continue;

        SIZE_T bytesRead = 0;
        status = SysNtReadVirtualMemory(processHandle, memoryInfo.BaseAddress, buffer, memoryInfo.RegionSize, &bytesRead);
        if (NT_SUCCESS(status))
        {
            const wchar_t* dataPtr = reinterpret_cast<const wchar_t*>(buffer);
            size_t         wordCount = bytesRead / sizeof(wchar_t);

            size_t foundPos = std::wstring_view(dataPtr, wordCount).find(wstr);

            if (foundPos != std::wstring_view::npos)
            {
                found = true;
                LPVOID lpFlaggedAddress = static_cast<LPBYTE>(memoryInfo.BaseAddress) + foundPos * sizeof(wchar_t);
                if ((DWORD64)lpFlaggedAddress != (DWORD64)memoryString.data())
                {
                    SharedUtil::AddDebugLog("Found at 0x%p | 0x%p", lpFlaggedAddress, (DWORD64)memoryString.data());
                }
                break;
            }
        }

        SysNtFreeVirtualMemory(currentProcess, &buffer, &allocationSize, MEM_RELEASE);
        if (found)
            break;
    }

    SysNtClose(processHandle);
}

void CHeuristicGuard::DoPulse()
{
    while (true)
    {
        std::vector<DWORD> pids = {GetCurrentProcessId()};

        LARGE_INTEGER frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        ScanForString();

        QueryPerformanceCounter(&end);
        float elapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
        SharedUtil::AddDebugLog("[+] Scan completed in %.4fs");

        Sleep(500);
    }
}