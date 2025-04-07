#include "StdInc.h"

namespace RuntimeImportResolver
{
    typedef LPVOID(WINAPI* f_VirtualAllocEx)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
    typedef LPVOID(WINAPI* f_VirtualFreeEx)(HANDLE, LPVOID, SIZE_T, DWORD);
    typedef LPVOID(WINAPI* f_VirtualProtectEx)(HANDLE, LPVOID, SIZE_T, DWORD, LPDWORD);
    typedef LPVOID(WINAPI* f_VirtualQueryEx)(HANDLE, LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T);
    typedef LPVOID(WINAPI* f_CreateRemoteThread)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
    typedef BOOL(WINAPI* f_WriteProcessMemory)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
    typedef BOOL(WINAPI* f_ReadProcessMemory)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
    typedef BOOL(__cdecl* f_RtlAddFunctionTable)(PRUNTIME_FUNCTION, DWORD, DWORD, DWORD, DWORD, DWORD);

    f_VirtualAllocEx VirtualAllocEx = nullptr;
    f_VirtualFreeEx  VirtualFreeEx = nullptr;
    f_VirtualProtectEx VirtualProtectEx = nullptr;
    f_VirtualQueryEx   VirtualQueryEx = nullptr;
    f_CreateRemoteThread CreateRemoteThread = nullptr;
    f_WriteProcessMemory WriteProcessMemory = nullptr;
    f_ReadProcessMemory ReadProcessMemory = nullptr;
    f_RtlAddFunctionTable RtlAddFunctionTable = nullptr;


    LPVOID ResolveFunction(const char* szLibrary, const char* szFunctionName)
    {
        HMODULE hModule = GetModuleHandleA(szLibrary);
        if (hModule)
        {
            FARPROC pFunction = GetProcAddress(hModule, szFunctionName);
            if (pFunction)
            {
                return pFunction;
            }
            else
            {
                SharedUtil::AddDebugLog("Failed to resolve function %s in library %s. Error: %lu\n", szFunctionName, szLibrary, GetLastError());
            }
        }
        return nullptr;
        
    }

    void ResolveCurrentImports()
    {
        VirtualAllocEx = (f_VirtualAllocEx)ResolveFunction("kernel32.dll", "VirtualAllocEx");
        VirtualFreeEx = (f_VirtualFreeEx)ResolveFunction("kernel32.dll", "VirtualFreeEx");
        VirtualProtectEx = (f_VirtualProtectEx)ResolveFunction("kernel32.dll", "VirtualProtectEx");
        VirtualQueryEx = (f_VirtualQueryEx)ResolveFunction("kernel32.dll", "VirtualQueryEx");
        CreateRemoteThread = (f_CreateRemoteThread)ResolveFunction("kernel32.dll", "CreateRemoteThread");
        WriteProcessMemory = (f_WriteProcessMemory)ResolveFunction("kernel32.dll", "WriteProcessMemory");
        ReadProcessMemory = (f_ReadProcessMemory)ResolveFunction("kernel32.dll", "ReadProcessMemory");
        RtlAddFunctionTable = (f_RtlAddFunctionTable)ResolveFunction("ntdll.dll", "RtlAddFunctionTable");
    }

}            // namespace RuntimeImportResolver