#include "StdInc.h"

#define START_ADDRESS (PVOID)0x00000000010000
#define END_ADDRESS   (0x00007FF8F2580000 - 0x00000000010000)

CMemoryGuard::CMemoryGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
    m_dwCurrentAddress = (DWORD)START_ADDRESS;
    m_dwMaxAddress = NULL;
}

CMemoryGuard::~CMemoryGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
}

void CMemoryGuard::DoPulse()
{
    while (true)
    {
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
                            SMemoryDetectionReport Report;
                            Report.AllocatedBase = info.AllocationBase;
                            Report.AllocatedProtect = info.AllocationProtect;
                            Report.RegionSize = info.RegionSize;

                            SharedUtil::AddDebugLog("Unregistred IAT At: 0x%x from 0x%X", z, dwModuleBase);

                            g_pSafeAntiCheat->NotifyDetection(eDetectionType::UNRECOGNISED_IAT_FOUND, &Report);
                            break;
                        }
                    }
                }
            }
            pCurrentAddress = (const void*)((const char*)(info.BaseAddress) + info.RegionSize);
        }
    }
}