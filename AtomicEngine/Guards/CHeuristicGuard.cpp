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
            wprintf(L"Add %s\n", Signature.c_str());
            m_vSignatures.push_back(Signature);
        }
    }
    zebii();
}

void CHeuristicGuard::zebii()
{
    int iCurrentSignature = 0;

    while (true)
    {
        if (m_vSignatures.empty())
            continue;

        for (std::wstring memoryString : m_vSignatures)
        {
            LARGE_INTEGER frequency, start, end;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start);

            std::wstring      c = Utils::CaesarDecrypt(memoryString, 3);
            std::wstring_view wstr(c.begin(), c.end());
            wprintf(L"current sig %d %s\n", iCurrentSignature, c.c_str());

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

                SIZE_T allocationSize = memoryInfo.RegionSize + wstr.size() * sizeof(wchar_t) - 1;
                PVOID  buffer = nullptr;
                status = SysNtAllocateVirtualMemory(hProcess, &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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
                        LPVOID lpFlaggedAddress = static_cast<LPBYTE>(memoryInfo.BaseAddress) + foundPos * sizeof(wchar_t);
                        if ((DWORD64)lpFlaggedAddress != (DWORD64)wstr.data() && !IsAddressInVector(m_vSignatures, lpFlaggedAddress) &&
                            (DWORD64)lpFlaggedAddress != (DWORD64)c.data())
                        {
                            SharedUtil::AddDebugLog("Found at 0x%p | 0x%p", lpFlaggedAddress, c.data());
                            g_pAtomicAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"string", std::string(wstr.begin(), wstr.end())},
                                                                                        //{"buffer", SharedUtil::Base64Encode(buf)},
                                                                                        {"memory_address", (DWORD64)lpFlaggedAddress},
                                                                                        {"region_size", memoryInfo.RegionSize},
                                                                                        {"base_address", (DWORD64)memoryInfo.BaseAddress},
                                                                                        {"allocation_protect", (DWORD64)memoryInfo.AllocationProtect},
                                                                                        {"allocation_address", (DWORD64)memoryInfo.AllocationBase}});
                            break;
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
