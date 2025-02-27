#include <fstream>
#include <iostream>
#include "StdInc.h"
#include "KernelCalls.hpp"

std::vector<std::wstring> m_vSignatures;

CHeuristicGuard::CHeuristicGuard()
{
}

CHeuristicGuard::~CHeuristicGuard()
{
}

void CHeuristicGuard::Initialize()
{
}

bool IsAddressInVector(const std::vector<std::wstring>& vec, const void* address)
{
    for (const auto& element : vec)
    {
        if ((DWORD64)element.data() == (DWORD64)address)
        {
            return true;
        }
    }
    return false;
}

void CHeuristicGuard::AddSignatures(std::map<std::string, std::vector<std::wstring>>& Signatures)
{
    for (auto& [name, vector] : Signatures)
    {
        for (auto& Signature : vector)
        {
            m_vSignatures.push_back(Signature);
        }
    }
    CAtomicThread::Create(&CHeuristicGuard::zebii, this);
}

void CHeuristicGuard::zebii()
{
    int iCurrentSignature = 0;

    while (true)
    {
        while (!g_pAtomicAntiCheat->RunScanners())
            ;

        if (m_vSignatures.empty())
            continue;

        HANDLE                        hProcess = GetCurrentProcess();
        NTSTATUS                      status;
        KernelCalls_OBJECT_ATTRIBUTES objAttr{};
        KernelCalls_CLIENT_ID         clientId{};
        HANDLE                        processHandle = GetCurrentProcess();

        RtlSecureZeroMemory(&objAttr, sizeof(KernelCalls_OBJECT_ATTRIBUTES));
        objAttr.Length = sizeof(KernelCalls_OBJECT_ATTRIBUTES);
        RtlSecureZeroMemory(&clientId, sizeof(KernelCalls_CLIENT_ID));
        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(GetCurrentProcessId()));

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        //for (std::wstring memoryString : m_vSignatures)
        std::string memoryString = "lpjxl_lpso_zlq32";
        {
            LARGE_INTEGER frequency, start, end;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start);

            std::string      c = Utils::CaesarDecrypt(memoryString, 3);
            std::string_view wstr(c.begin(), c.end());

            MEMORY_BASIC_INFORMATION memoryInfo{};
            bool                     found = false;

            for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
                 addr = static_cast<LPBYTE>(memoryInfo.BaseAddress) + memoryInfo.RegionSize)
            {
                PVOID  baseAddress = addr;
                SIZE_T regionSize = sizeof(memoryInfo);
                SIZE_T returnLength;

                status = SysNtQueryVirtualMemory(processHandle, baseAddress, MemoryBasicInformation, &memoryInfo, regionSize, &returnLength);
                if (!NT_SUCCESS(status) || /*!(memoryInfo.State | MEM_PRIVATE) ||*/ memoryInfo.Protect & PAGE_READWRITE)
                    continue;

                SIZE_T allocationSize = memoryInfo.RegionSize + wstr.size() * sizeof(char) - 1;
                PVOID  buffer = nullptr;
                status = SysNtAllocateVirtualMemory(hProcess, &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (!NT_SUCCESS(status))
                    continue;

                SIZE_T bytesRead = 0;
                status = SysNtReadVirtualMemory(processHandle, memoryInfo.BaseAddress, buffer, memoryInfo.RegionSize, &bytesRead);
                if (NT_SUCCESS(status))
                {
                    const char* dataPtr = reinterpret_cast<const char*>(buffer);
                    size_t         wordCount = bytesRead / sizeof(char);
                     
                    size_t foundPos = std::string_view(dataPtr, wordCount).find(wstr);

                    if (foundPos != std::wstring_view::npos)
                    {
                        LPVOID lpFlaggedAddress = static_cast<LPBYTE>(memoryInfo.BaseAddress) + foundPos * sizeof(char);
                        if ((DWORD64)lpFlaggedAddress != (DWORD64)wstr.data() && !IsAddressInVector(m_vSignatures, lpFlaggedAddress) &&
                            (DWORD64)lpFlaggedAddress != (DWORD64)c.data())
                        {
                            SharedUtil::AddDebugLog("Found at 0x%p | 0x%p", lpFlaggedAddress, c.data());
                            
                            g_pAtomicAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"string", std::string(wstr.begin(), wstr.end())},
                                                                                  //      {"buffer", SharedUtil::Base64Encode(std::wstring(dataPtr + foundPos, 768))},
                                                                                        {"memory_address", (DWORD64)lpFlaggedAddress},
                                                                                        {"region_size", memoryInfo.RegionSize},
                                                                                        {"base_address", (DWORD64)memoryInfo.BaseAddress},
                                                                                        {"region_type", (DWORD64)memoryInfo.Type},
                                                                                        {"region_state", (DWORD64)memoryInfo.State},
                                                                                        {"region_protect", (DWORD64)memoryInfo.Protect},
                                                                                        {"allocation_protect", (DWORD64)memoryInfo.AllocationProtect},
                                                                                        {"allocation_address", (DWORD64)memoryInfo.AllocationBase}});
                        }
                    }
                }

                SysNtFreeVirtualMemory(hProcess, &buffer, &allocationSize, MEM_RELEASE);
                if (found)
                    break;
            }

            SysNtClose(processHandle);

            QueryPerformanceCounter(&end);
            float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
            SharedUtil::AddDebugLog("[+] Scan completed in %.5fs", fElapsedTime);

            iCurrentSignature++;          
        }
    }
}
