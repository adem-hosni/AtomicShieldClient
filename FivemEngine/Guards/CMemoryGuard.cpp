#include "StdInc.h"

CMemoryGuard::CMemoryGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
    m_dwCurrentAddress = NULL;
    m_dwMaxAddress = NULL;
}

CMemoryGuard::~CMemoryGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
}

void CMemoryGuard::DoPulse()
{
    DWORD64 dwEnd = m_dwCurrentAddress + m_dwMaxAddress;
    DWORD   mask = (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ);

    //NT_SUCCESS(NtQueryVirtualMemory(GetCurrentProcess(), reinterpret_cast<PVOID>(m_dwCurrentAddress), MemoryBasicInformation, &m_mbi, sizeof(m_mbi), 0))
    SharedUtil::AddDebugLog("Begin Memory Guard Scan");
    while (m_dwCurrentAddress < dwEnd && VirtualQuery((void*)m_dwCurrentAddress, &m_mbi, sizeof(m_mbi)))
    {
        if ((m_mbi.State != MEM_FREE && m_mbi.State != MEM_RELEASE) && m_mbi.Protect & mask)
        {
            BYTE    completeSequence = 0;
            DWORD64 dwFoundIAT = 0x0;
            for (DWORD64 z = m_dwCurrentAddress; z < m_dwCurrentAddress + m_mbi.RegionSize; z++)
            {
                for (DWORD x = 0; x < (8 * 6); x += 0x6)
                {
                    if ((x + z) < m_dwCurrentAddress + m_mbi.RegionSize && (x + z + 0x1) < m_dwCurrentAddress + m_mbi.RegionSize)
                    {
                        if ((*(BYTE*)(z + x) == 0xFF && *(BYTE*)(x + z + 0x1) == 0x25))
                        {
                            dwFoundIAT = (x + z);
                            completeSequence++;
                        }
                        else
                        {
                            completeSequence = 0;
                        }
                    }
                }

                if (completeSequence >= 8)
                {
                    SharedUtil::AddDebugLog("Completed sequence 0x%x", completeSequence);
                    completeSequence = NULL;
                    char szMappedName[256];
                    memset(szMappedName, 0, sizeof(szMappedName));
                    GetMappedFileName(g_pSafeAntiCheat->GetProcessHandle(), m_mbi.BaseAddress, szMappedName, sizeof(szMappedName));
                    std::string strPossibleModuleName = Utils::ParseModuleNameFromPath(szMappedName);
                    bool        bCloaked = false;
                    if (!Utils::IsAddressInModuledRange((DWORD)m_mbi.BaseAddress, strPossibleModuleName, &bCloaked))
                    {
                        SharedUtil::AddDebugLog("Detected cloaked module at 0x%x (possible module name: %s)", strPossibleModuleName.c_str());
                    }
                }
            }
        }
        m_dwCurrentAddress = (DWORD)m_mbi.BaseAddress + m_mbi.RegionSize;
    }
    SharedUtil::AddDebugLog("End Memory Guard Scan");
}