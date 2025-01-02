#include "StdInc.h"

#define START_ADDRESS (PVOID)0x00000000010000
#define END_ADDRESS   (0x00007FF8F2580000 - 0x00000000010000)

CMemoryGuard::CMemoryGuard()
{
}

CMemoryGuard::~CMemoryGuard()
{
}

void CMemoryGuard::DoPulse()
{
    while (true)
    {
        int founds = 0;
        SharedUtil::AddDebugLog("Begin Memory Guard Scan");
        int                      iDetectionCount = 0;
        DWORD                    mask = (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ);
        MEMORY_BASIC_INFORMATION info;
        const void*              pCurrentAddress = START_ADDRESS;
        const void*              end = (const void*)((const char*)pCurrentAddress + END_ADDRESS);

        while (pCurrentAddress < end && VirtualQuery(pCurrentAddress, &info, sizeof(info)) == sizeof(info))
        {
            if ((info.State != MEM_FREE && info.State != MEM_RELEASE) && info.Type & (MEM_IMAGE | MEM_PRIVATE) && info.Protect & mask)
            {
                DWORD64 dwModuleBase = Utils::IsAddressInModuledRange((DWORD64)pCurrentAddress);
                if (dwModuleBase == -1)
                {
                    for (DWORD_PTR z = (DWORD_PTR)pCurrentAddress; z < ((DWORD_PTR)pCurrentAddress + info.RegionSize); z++)
                    {
                        bool bCompletedSequence = false;
                        __try
                        {
                            for (DWORD x = 0; x < (10 * 6); x += 0x6)
                                bCompletedSequence = *(byte*)(z + x) == 0xFF && *(byte*)(x + z + 0x1) == 0x25;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                        };
                        if (bCompletedSequence)
                        {
                            iDetectionCount++;

                            SMemoryDetectionReport Report;
                            Report.AllocatedBase = info.AllocationBase;
                            Report.AllocatedProtect = info.AllocationProtect;
                            Report.RegionSize = info.RegionSize;

                            SharedUtil::AddDebugLog("Unregistred IAT At: 0x%p (%d) base address: 0x%p allocation base: 0x%p", z, iDetectionCount,
                                                    (DWORD64)info.BaseAddress, (DWORD64)info.AllocationBase);

                            HMODULE hModule = NULL;
                            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                                   (wchar_t*)info.BaseAddress, &hModule) &&
                                hModule == NULL)
                            {
                                const char pattern[] = {
                                    "\x48\x8B\xC4\x48\x89\x58\x20\x4C\x89\x40\x18\x89\x50\x10\x48\x89\x48\x08\x56\x57\x41\x56\x48\x83\xEC\x40\x49\x8B\xF0\x8B"
                                    "\xFA"
                                    "\x4C\x8B\xF1\x85\xD2\x75\x0F\x39\x15\x00\x00\x00\x00\x7F\x07\x33\xC0\xE9\x00\x00\x00\x00"};
                                const char wildcard[] = {"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxx????"};

                                bool  found = false;
                                DWORD patternLength = (DWORD)strlen(wildcard);
                                __try
                                {
                                    for (DWORD j = 0; j < patternLength; j++)
                                    {
                                        found &= wildcard[j] == '?' || pattern[j] == *(char*)(z + j);
                                    }
                                }
                                __except (EXCEPTION_EXECUTE_HANDLER)
                                {
                                };

                                if (found)
                                {
                                    SharedUtil::AddDebugLog("Injected module!");
                                }
                            }
                            else
                            {
                                __try
                                {
                                    IMAGE_DOS_HEADER* dosHeader = static_cast<IMAGE_DOS_HEADER*>(info.BaseAddress);
                                    if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE)
                                    {
                                        SharedUtil::AddDebugLog("Valid PE Header!");
                                    }
                                }
                                __except (EXCEPTION_EXECUTE_HANDLER)
                                {
                                }
                            }

                            // g_pSafeAntiCheat->NotifyDetection(eDetectionType::UNRECOGNISED_IAT_FOUND, &Report);
                            break;
                        }
                    }
                }
            }
            pCurrentAddress = (const void*)((const char*)(info.BaseAddress) + info.RegionSize);
        }
        SharedUtil::AddDebugLog("End Memory Guard Scan");
    }
}