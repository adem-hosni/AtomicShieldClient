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


bool CHeuristicGuard::IsWhitelistedModule(HANDLE hProcess, LPVOID address)
{
    HMODULE hMods[1024];
    DWORD   cbNeeded;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
    {
        for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
        {
            WCHAR szModName[MAX_PATH];
            if (GetModuleFileNameExW(hProcess, hMods[i], szModName, MAX_PATH))
            {
                std::wstring modulePath = szModName;
                size_t       pos = modulePath.find_last_of(L"\\/");

                std::wstring moduleName = (pos != std::wstring::npos) ? modulePath.substr(pos + 1) : modulePath;

                // Log each module being considered
                SharedUtil::AddDebugLog("[Whitelist] Checking module: %ws", moduleName.c_str());

                if (m_whitelistedModules.find(moduleName) != m_whitelistedModules.end())
                {
                    MODULEINFO modInfo;
                    if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo)))
                    {
                        LPBYTE modStart = reinterpret_cast<LPBYTE>(hMods[i]);
                        LPBYTE modEnd = modStart + modInfo.SizeOfImage;

                        if (address >= modStart && address < modEnd)
                        {
                            SharedUtil::AddDebugLog("[Whitelist] Address 0x%p is inside whitelisted module: %ws", address, moduleName.c_str());
                            return true;
                        }
                        else
                        {
                            SharedUtil::AddDebugLog("[Whitelist] Address 0x%p is not inside module range: %ws [0x%p - 0x%p]", address, moduleName.c_str(),
                                                    modStart, modEnd);
                        }
                    }
                }
                else
                {
                    SharedUtil::AddDebugLog("[Whitelist] Module not whitelisted: %ws", moduleName.c_str());
                }
            }
        }
    }
    else
    {
        SharedUtil::AddDebugLog("[-] EnumProcessModules failed.");
    }

    return false;
}


#pragma optimize("", off)
void CHeuristicGuard::DoPulse()
{
    KernelCalls_OBJECT_ATTRIBUTES objAttr{};
    KernelCalls_CLIENT_ID         clientId{};
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);

    RtlSecureZeroMemory(&objAttr, sizeof(KernelCalls_OBJECT_ATTRIBUTES));
    objAttr.Length = sizeof(KernelCalls_OBJECT_ATTRIBUTES);
    RtlSecureZeroMemory(&clientId, sizeof(KernelCalls_CLIENT_ID));

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    HANDLE   processHandle;
    NTSTATUS status;

    constexpr SIZE_T MAX_REGION_SIZE = 10 * 1024 * 1024;       

    PVOID  pReuseBuffer = nullptr;
    SIZE_T bufferSize = MAX_REGION_SIZE;
    status = SysNtAllocateVirtualMemory(GetCurrentProcess(), &pReuseBuffer, 0, &bufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!NT_SUCCESS(status) || !pReuseBuffer)
    {
        SharedUtil::AddDebugLog("[-] Failed to allocate reuse buffer.");
        return;
    }

    while (g_pAtomicAntiCheat->RunScanners())
    {
        while (g_pAtomicAntiCheat->GetProcessID() == NULL)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        clientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(g_pAtomicAntiCheat->GetProcessID()));
        status = SysNtOpenProcess(&processHandle, PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, &objAttr, &clientId);
        if (!NT_SUCCESS(status))
            continue;

        for (const auto& decryptedStr : m_vSignatures)
        {
            LARGE_INTEGER frequency, start, end;
            QueryPerformanceFrequency(&frequency);
            QueryPerformanceCounter(&start);

            MEMORY_BASIC_INFORMATION memoryInfo{};

for (LPVOID addr = sysInfo.lpMinimumApplicationAddress; addr < sysInfo.lpMaximumApplicationAddress;
                 addr = static_cast<LPBYTE>(memoryInfo.BaseAddress) + memoryInfo.RegionSize)
            {
                PVOID  baseAddress = addr;
                SIZE_T returnLength = 0;

                status = SysNtQueryVirtualMemory(processHandle, baseAddress, MemoryBasicInformation, &memoryInfo, sizeof(memoryInfo), &returnLength);
                if (!NT_SUCCESS(status) || memoryInfo.State != MEM_COMMIT || memoryInfo.Type != MEM_PRIVATE ||
                    memoryInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD | PAGE_READONLY))
                    continue;

                //if (IsWhitelistedModule(processHandle, memoryInfo.BaseAddress))
                //    continue;

                SIZE_T allocationSize = memoryInfo.RegionSize;
                if (allocationSize > MAX_REGION_SIZE)
                    continue;

                SIZE_T bytesRead = 0;
                status = SysNtReadVirtualMemory(processHandle, memoryInfo.BaseAddress, pReuseBuffer, allocationSize, &bytesRead);
                if (!NT_SUCCESS(status) || bytesRead == 0 || bytesRead > MAX_REGION_SIZE)
                    continue;

                const char*      dataPtr = reinterpret_cast<const char*>(pReuseBuffer);
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
            }

            QueryPerformanceCounter(&end);
            float fElapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / frequency.QuadPart;
            SharedUtil::AddDebugLog("[+] Scan completed in %.5fs", fElapsedTime);

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        SysNtClose(processHandle);
    }

    // Free buffer once
    if (pReuseBuffer)
    {
        SysNtFreeVirtualMemory(GetCurrentProcess(), &pReuseBuffer, &bufferSize, MEM_RELEASE);
        pReuseBuffer = nullptr;
    }

    _endthreadex(0);
}
#pragma optimize("", on)
