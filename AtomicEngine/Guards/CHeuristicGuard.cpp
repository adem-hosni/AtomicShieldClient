#include <fstream>
#include <iostream>
#include "StdInc.h"
#include "KernelCalls.hpp"
#include <algorithm>

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
}

void CHeuristicGuard::Initialize()
{
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::vector<std::string>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            m_vSignatures.push_back(Utils::CaesarDecrypt(Signature, 3));
        }
    }
}

#pragma optimize("", off)
void CHeuristicGuard::DoPulse()
{
    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
    KernelCalls_CLIENT_ID         clientId{};
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);

    RtlSecureZeroMemory(&objAttr, sizeof(KernelCalls_OBJECT_ATTRIBUTES));
    objAttr.Length = sizeof(KernelCalls_OBJECT_ATTRIBUTES);
    RtlSecureZeroMemory(&clientId, sizeof(KernelCalls_CLIENT_ID));

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    HANDLE   processHandle;
    NTSTATUS status;

    char buffer[512000];

    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(g_pAtomicAntiCheat->GetProcessID()));

        status = SysNtOpenProcess(&processHandle, (0x0400) | (0x0010), &objAttr, &clientId);
        if (!NT_SUCCESS(status))
            continue;

        for (const auto& decryptedStr : m_vSignatures)
        {
            LARGE_INTEGER frequency, start, end;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start);

            MEMORY_BASIC_INFORMATION memoryInfo{};
            bool                     found = false;

            for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
                 addr = static_cast<LPBYTE>(memoryInfo.BaseAddress) + memoryInfo.RegionSize)
            {
                PVOID  baseAddress = addr;
                SIZE_T regionSize = sizeof(memoryInfo);
                SIZE_T returnLength = 0;

                status = SysNtQueryVirtualMemory(processHandle, baseAddress, MemoryBasicInformation, &memoryInfo, regionSize, &returnLength);
                if (!NT_SUCCESS(status) || memoryInfo.State != MEM_COMMIT || memoryInfo.Protect == PAGE_NOACCESS ||
                    memoryInfo.Protect & (PAGE_GUARD | PAGE_NOACCESS | PAGE_READONLY) || memoryInfo.Type != MEM_PRIVATE)
                    continue;

                SIZE_T allocationSize = memoryInfo.RegionSize;

                // Allocate memory safely
                /*PVOID buffer = nullptr;
                status = SysNtAllocateVirtualMemory(GetCurrentProcess(), &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (!NT_SUCCESS(status) || buffer == nullptr)
                {
                    buffer = nullptr;
                    continue;
                }*/

                SIZE_T bytesRead = 0;
                memset(buffer, 0, sizeof(buffer));
                status = SysNtReadVirtualMemory(processHandle, memoryInfo.BaseAddress, buffer, allocationSize, &bytesRead);

                if (!NT_SUCCESS(status) || bytesRead == 0 || bytesRead > allocationSize)
                {
                    SharedUtil::AddDebugLog("Failed to read virtual memory 0x%x 0x%x (Allocation Size: %d)", status, GetLastError(), allocationSize);
                    //SysNtFreeVirtualMemory(processHandle, &buffer, &allocationSize, MEM_RELEASE);
                    continue;
                }

                const char*      dataPtr = reinterpret_cast<const char*>(buffer);
                std::string_view memoryView(dataPtr, bytesRead);

                if (!decryptedStr.empty() && decryptedStr.find_first_not_of(" \t\n\r\0") != std::string::npos)
                {
                    size_t foundPos = memoryView.find(decryptedStr);
                    if (foundPos != std::string_view::npos)
                    {
                        LPVOID lpFlaggedAddress = static_cast<LPBYTE>(memoryInfo.BaseAddress) + foundPos;
                        SharedUtil::AddDebugLog("Found at 0x%p", lpFlaggedAddress);

                        g_pAtomicAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"string", decryptedStr},
                                                                                    {"memory_address", (DWORD64)lpFlaggedAddress},
                                                                                    {"region_size", memoryInfo.RegionSize},
                                                                                    {"base_address", (DWORD64)memoryInfo.BaseAddress},
                                                                                    {"region_type", (DWORD64)memoryInfo.Type},
                                                                                    {"region_state", (DWORD64)memoryInfo.State},
                                                                                    {"region_protect", (DWORD64)memoryInfo.Protect},
                                                                                    {"allocation_protect", (DWORD64)memoryInfo.AllocationProtect},
                                                                                    {"allocation_address", (DWORD64)memoryInfo.AllocationBase}});

                        g_pAtomicAntiCheat->RunScanners(false);
                    }
                }

                // Safe memory free
                /*if (buffer)
                {
                    SysNtFreeVirtualMemory(GetCurrentProcess(), &buffer, &allocationSize, MEM_RELEASE);
                    buffer = nullptr;
                }*/
            }

            QueryPerformanceCounter(&end);
            float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;

            //int sleepTime = std::max<int>(1, static_cast<int>((fElapsedTime * 1000) / 4));
            SharedUtil::AddDebugLog("[+] Scan completed in %.5fs",fElapsedTime);

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
        SysNtClose(processHandle);
    }

    _endthreadex(0);
}
#pragma optimize("", on)
