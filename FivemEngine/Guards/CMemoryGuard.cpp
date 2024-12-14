#include "StdInc.h"

CMemoryGuard::CMemoryGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
}

CMemoryGuard::~CMemoryGuard()
{
    m_mbi = {0};
    m_systemInfo = {0};
}

void CMemoryGuard::Initialize()
{
    GetNativeSystemInfo(&m_systemInfo);
    m_dwMaxAddress = (DWORD64)m_systemInfo.lpMaximumApplicationAddress - (DWORD64)m_systemInfo.lpMinimumApplicationAddress;
}

void CMemoryGuard::DoPulse()
{
    return; // Naf5an krarez
    DWORD64 dwEnd = m_dwCurrentAddress + m_dwMaxAddress;
    DWORD   mask = (PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_READ);

    while (m_dwCurrentAddress < dwEnd && NT_SUCCESS(NtQueryVirtualMemory(g_pSafeAntiCheat->GetProcessHandle(), reinterpret_cast<PVOID>(m_dwCurrentAddress),
                                                                         MemoryBasicInformation, &m_mbi, sizeof(m_mbi), 0)))
    {
        SharedUtil::AddDebugLog("State: 0x%x Protect: 0x%x", m_mbi.State, m_mbi.Protect);
        if ((m_mbi.State != MEM_FREE && m_mbi.State != MEM_RELEASE) && m_mbi.Protect & mask)
        {
            SharedUtil::AddDebugLog("Valid memory state");

            BYTE    completeSequence = 0;
            DWORD64 dwFoundIAT = 0x0;
            BYTE    byte1;
            BYTE    byte2;
            for (DWORD64 z = m_dwCurrentAddress; z < m_dwCurrentAddress + m_mbi.RegionSize; z++)
            {
                for (DWORD x = 0; x < (8 * 6); x += 0x6)
                {
                    if ((x + z) < m_dwCurrentAddress + m_mbi.RegionSize && (x + z + 0x1) < m_dwCurrentAddress + m_mbi.RegionSize)
                    {
                        ReadProcessMemory(g_pSafeAntiCheat->GetProcessHandle(), (LPVOID)(z + x), &byte1, sizeof(byte1), nullptr);
                        ReadProcessMemory(g_pSafeAntiCheat->GetProcessHandle(), (LPVOID)(z + x + 0x1), &byte2, sizeof(byte2), nullptr);

                        if (byte1 == 0xFF && byte2 == 0x25)
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
                else
                {
                    SharedUtil::AddDebugLog("Uncompleted sequence 0x%x", completeSequence);
                }
            }
        }
        m_dwCurrentAddress = (DWORD)m_mbi.BaseAddress + m_mbi.RegionSize;
    }
}