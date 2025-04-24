#include <Windows.h>
#include "skCrypter.h"

namespace RuntimeImportResolver
{
    typedef LPVOID (*f_VirtualAllocEx)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
    typedef LPVOID (*f_VirtualFreeEx)(HANDLE, LPVOID, SIZE_T, DWORD);
    typedef LPVOID (*f_VirtualProtectEx)(HANDLE, LPVOID, SIZE_T, DWORD, LPDWORD);
    typedef LPVOID (*f_VirtualQueryEx)(HANDLE, LPCVOID, PMEMORY_BASIC_INFORMATION, SIZE_T);
    typedef LPVOID (*f_CreateRemoteThread)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
    typedef BOOL (*f_WriteProcessMemory)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
    typedef BOOL (*f_ReadProcessMemory)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
    typedef BOOL(__cdecl* f_RtlAddFunctionTable)(PRUNTIME_FUNCTION, DWORD, DWORD, DWORD, DWORD, DWORD);
    typedef int(__stdcall* f_gethostname)(char*, int);

    inline f_VirtualAllocEx      VirtualAllocEx = nullptr;
    inline f_VirtualFreeEx       VirtualFreeEx = nullptr;
    inline f_VirtualProtectEx    VirtualProtectEx = nullptr;
    inline f_VirtualQueryEx      VirtualQueryEx = nullptr;
    inline f_CreateRemoteThread  CreateRemoteThread = nullptr;
    inline f_WriteProcessMemory  WriteProcessMemory = nullptr;
    inline f_ReadProcessMemory   ReadProcessMemory = nullptr;
    inline f_RtlAddFunctionTable RtlAddFunctionTable = nullptr;
    inline f_gethostname         gethostname = nullptr;

    static LPVOID ResolveFunction(const char* szLibrary, const char* szFunctionName)
    {
        HMODULE hModule = LoadLibrary(szLibrary);
        if (!hModule)
        {
            DWORD error = GetLastError();
            printf("Failed to load library %s. Error: %lu", szLibrary, error);
            return nullptr;
        }

        FARPROC pFunction = GetProcAddress(hModule, szFunctionName);
        if (!pFunction)
        {
            DWORD error = GetLastError();
            printf("Failed to resolve function %s in %s. Error: %lu\n", szFunctionName, szLibrary, error);
            return nullptr;
        }

        printf("Resolved %s!%s to 0x%p\n", szLibrary, szFunctionName, pFunction);
        return pFunction;
    }

    static void ResolveCurrentImports()
    {
        RuntimeImportResolver::VirtualAllocEx = (f_VirtualAllocEx)ResolveFunction(skCrypt("kernel32.dll"), skCrypt("VirtualAllocEx"));
        RuntimeImportResolver::VirtualFreeEx = (f_VirtualFreeEx)ResolveFunction(skCrypt("kernel32.dll"), skCrypt("VirtualFreeEx"));
        RuntimeImportResolver::VirtualProtectEx = (f_VirtualProtectEx)ResolveFunction(skCrypt("kernel32.dll"), skCrypt("VirtualProtectEx"));
        RuntimeImportResolver::VirtualQueryEx = (f_VirtualQueryEx)ResolveFunction(skCrypt("kernel32.dll"), skCrypt("VirtualQueryEx"));
        RuntimeImportResolver::CreateRemoteThread =
            (f_CreateRemoteThread)ResolveFunction(skCrypt("kernel32.dll"), skCrypt("CreateRemoteThread"));
        RuntimeImportResolver::WriteProcessMemory =
            (f_WriteProcessMemory)ResolveFunction(skCrypt("kernel32.dll"), skCrypt("WriteProcessMemory"));
        RuntimeImportResolver::ReadProcessMemory =
            (f_ReadProcessMemory)ResolveFunction(skCrypt("kernel32.dll"), skCrypt("ReadProcessMemory"));
        RuntimeImportResolver::RtlAddFunctionTable = (f_RtlAddFunctionTable)ResolveFunction(skCrypt("ntdll.dll"), skCrypt("RtlAddFunctionTable"));
        RuntimeImportResolver::gethostname = (f_gethostname)ResolveFunction(skCrypt("Ws2_32.dll"), skCrypt("gethostname"));
    }

}            // namespace RuntimeImportResolvers