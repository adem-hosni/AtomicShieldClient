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
    //CAtomicThread::Create(&CHeuristicGuard::zebii, this);
}
void CHeuristicGuard::DoPulse()
{
    int iCurrentSignature = 0;

    while (true)
    {
        while (!g_pAtomicAntiCheat->RunScanners())
        {
            Sleep(50);
        }
        DWORD                         pid = SharedUtil::GetFivemProcessID();
        HANDLE                        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        NTSTATUS                      status;
        KernelCalls_OBJECT_ATTRIBUTES objAttr{};
        KernelCalls_CLIENT_ID         clientId{};

        RtlSecureZeroMemory(&objAttr, sizeof(KernelCalls_OBJECT_ATTRIBUTES));
        objAttr.Length = sizeof(KernelCalls_OBJECT_ATTRIBUTES);
        RtlSecureZeroMemory(&clientId, sizeof(KernelCalls_CLIENT_ID));
        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(pid));

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        std::vector<std::string> memoryStrings = {"lpjxl_lpso_zlq32", "dsl.wcsurmhfw.frp"};

        for (const auto& memoryString : memoryStrings)
        {
            LARGE_INTEGER frequency, start, end;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start);

            std::string decryptedStr = Utils::CaesarDecrypt(memoryString, 3);

            MEMORY_BASIC_INFORMATION memoryInfo{};
            bool                     found = false;

            for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
                 addr = static_cast<LPBYTE>(memoryInfo.BaseAddress) + memoryInfo.RegionSize)
            {
                PVOID  baseAddress = addr;
                SIZE_T regionSize = sizeof(memoryInfo);
                SIZE_T returnLength = 0;

                status = SysNtQueryVirtualMemory(hProcess, baseAddress, MemoryBasicInformation, &memoryInfo, regionSize, &returnLength);
                if (!NT_SUCCESS(status) || memoryInfo.State != MEM_COMMIT || memoryInfo.Protect == PAGE_NOACCESS)
                    continue;

                // Limit allocation to 1MB max
                SIZE_T allocationSize = min(memoryInfo.RegionSize, 1024 * 1024);

                // Allocate memory safely
                PVOID buffer = nullptr;
                status = SysNtAllocateVirtualMemory(hProcess, &buffer, 0, &allocationSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (!NT_SUCCESS(status) || buffer == nullptr)
                {
                    buffer = nullptr;
                    continue;
                }

                SIZE_T bytesRead = 0;
                status = SysNtReadVirtualMemory(hProcess, memoryInfo.BaseAddress, buffer, allocationSize, &bytesRead);

                if (!NT_SUCCESS(status) || bytesRead == 0 || bytesRead > allocationSize)
                {
                    SysNtFreeVirtualMemory(hProcess, &buffer, &allocationSize, MEM_RELEASE);
                    continue;
                }

                const char*      dataPtr = reinterpret_cast<const char*>(buffer);
                std::string_view memoryView(dataPtr, bytesRead);

                size_t foundPos = memoryView.find(decryptedStr);
                if (foundPos != std::string_view::npos)
                {
                    LPVOID lpFlaggedAddress = static_cast<LPBYTE>(memoryInfo.BaseAddress) + foundPos;
                    if ((DWORD64)lpFlaggedAddress != (DWORD64)decryptedStr.data() && !IsAddressInVector(m_vSignatures, lpFlaggedAddress))
                    {
                        SharedUtil::AddDebugLog("Found at 0x%p | 0x%p", lpFlaggedAddress, decryptedStr.data());

                        g_pAtomicAntiCheat->NotifyDetection(CHEAT_SIGNATURE_FOUND, {{"string", decryptedStr},
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

                // Safe memory free
                if (buffer)
                {
                    SysNtFreeVirtualMemory(hProcess, &buffer, &allocationSize, MEM_RELEASE);
                    buffer = nullptr;
                }

                if (found)
                    break;
            }

            SysNtClose(hProcess);

            QueryPerformanceCounter(&end);
            float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
            SharedUtil::AddDebugLog("[+] Scan for '%s' completed in %.5fs", memoryString.c_str(), fElapsedTime);

            iCurrentSignature++;
        }
    }
}
