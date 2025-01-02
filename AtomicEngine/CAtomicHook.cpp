#include "CAtomicHook.h"

CAtomicHook::CAtomicHook(LPVOID lpTargetFunction, LPVOID lpDetourFunction)
{
    m_bEnabled = false;
    m_lpTargetFunction = lpTargetFunction;
    m_lpDetourFunction = lpDetourFunction;
    m_lpOriginalFunction = nullptr;

    m_vOriginalBytes = {};

    ReadOriginalBytes();
    CreateTrampoline();
}

CAtomicHook::~CAtomicHook()
{
    if (m_lpTargetFunction)
        delete[] m_lpTargetFunction;
    if (m_lpDetourFunction)
        delete[] m_lpDetourFunction;
}

CAtomicHook* CAtomicHook::Create(LPVOID lpTargetFunction, LPVOID lpDetourFunction)
{
    CAtomicHook* pAtomicHook = new CAtomicHook(lpTargetFunction, lpDetourFunction);
    pAtomicHook->Enable();

    return pAtomicHook;
}

void CAtomicHook::ReadOriginalBytes()
{
    BYTE* targetBytes = static_cast<BYTE*>(m_lpTargetFunction);
    m_vOriginalBytes.assign(targetBytes, targetBytes + 5);
}

void CAtomicHook::Enable()
{
    if (m_bEnabled)
        return;

    DWORD dwOldProtect;
    if (!VirtualProtect(m_lpTargetFunction, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect))
    {
        SharedUtil::AddDebugLog("Failed to change memory protection at 0x%x", (DWORD64)m_lpTargetFunction);
        return;
    }

    DWORD64 dwRelativeAddress = reinterpret_cast<DWORD64>(m_lpDetourFunction) - reinterpret_cast<DWORD64>(m_lpTargetFunction) - 5;

    // Write the JMP Instruction and the relative address
    BYTE* targetBytes = (BYTE*)m_lpTargetFunction;
    targetBytes[0] = 0xE9;
    *reinterpret_cast<DWORD64*>(&targetBytes[1]) = dwRelativeAddress;

    if (!VirtualProtect(m_lpTargetFunction, 5, dwOldProtect, &dwOldProtect))
    {
        SharedUtil::AddDebugLog("Failed to restore memory protection! 0x%x", GetLastError());
    }
    
    m_bEnabled = true;
}

void CAtomicHook::CreateTrampoline()
{
    LPVOID lpTrampoline = VirtualAlloc(nullptr, m_vOriginalBytes.size() + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!lpTrampoline)
    {
        SharedUtil::AddDebugLog("Failed to allocate memory for trampoline! 0x%x", GetLastError());
        return;
    }

    // Copy the original bytes to the trampoline
    memcpy(lpTrampoline, m_lpTargetFunction, m_vOriginalBytes.size());

    // Add a jump back to the original function
    BYTE* trampolineBytes = (BYTE*)lpTrampoline;
    DWORD64 dwReturnAddress = reinterpret_cast<DWORD64>(m_lpTargetFunction) + m_vOriginalBytes.size();

    trampolineBytes[m_vOriginalBytes.size()] = 0xE9; // JMP opcode

    *reinterpret_cast<DWORD64*>(&trampolineBytes[m_vOriginalBytes.size() + 1]) = dwReturnAddress - reinterpret_cast<DWORD64>(lpTrampoline) - 5;

    m_lpOriginalFunction = lpTrampoline;
}