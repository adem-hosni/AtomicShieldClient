#pragma once
#include "StdInc.h"

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#if defined(DISABLE_OUTPUT)
    #define ILog(data, ...)
#else
    #define ILog(text, ...) SharedUtil::AddDebugLog(skCrypt(text), __VA_ARGS__);
#endif

#ifdef _WIN64
    #define CURRENT_ARCH IMAGE_FILE_MACHINE_AMD64
#else
    #define CURRENT_ARCH IMAGE_FILE_MACHINE_I386
#endif

#define RELOC_FLAG32(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_HIGHLOW)
#define RELOC_FLAG64(RelInfo) ((RelInfo >> 0x0C) == IMAGE_REL_BASED_DIR64)

#ifdef _WIN64
    #define RELOC_FLAG RELOC_FLAG64
#else
    #define RELOC_FLAG RELOC_FLAG32
#endif

namespace SelfMapModule
{
    using f_LoadLibraryA = HINSTANCE(WINAPI*)(const char* lpLibFilename);
    using f_GetProcAddress = FARPROC(WINAPI*)(HMODULE hModule, LPCSTR lpProcName);
    using f_DLL_ENTRY_POINT = BOOL(WINAPI*)(void* hDll, DWORD dwReason, void* pReserved);

#ifdef _WIN64
    using f_RtlAddFunctionTable = BOOL(WINAPIV*)(PRUNTIME_FUNCTION FunctionTable, DWORD EntryCount, DWORD64 BaseAddress);
#endif

    struct MANUAL_MAPPING_DATA
    {
        f_LoadLibraryA   pLoadLibraryA;
        f_GetProcAddress pGetProcAddress;
#ifdef _WIN64
        f_RtlAddFunctionTable pRtlAddFunctionTable;
#endif
        BYTE*     pbase;
        HINSTANCE hMod;
        DWORD     fdwReasonParam;
        LPVOID    reservedParam;
        BOOL      SEHSupport;
    };

    int MapModule(BYTE* pSrcData, SIZE_T FileSize, bool ClearHeader, bool ClearNonNeededSections, bool AdjustProtections, bool SEHExceptionSupport,
                  DWORD fdwReason, LPVOID lpReserved);
    void __stdcall Shellcode(MANUAL_MAPPING_DATA* pData);
};            // namespace SelfMapModule
