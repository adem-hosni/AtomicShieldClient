#pragma once
#include "StdInc.h"
#include <Windows.h>
#include <TlHelp32.h>

namespace Utils
{
    static std::map<DWORD64, DWORD64>    orderedMapping;             // global module runtime list (PE Image Info)
    static std::map<DWORD, std::wstring> orderedIdentify;            // global module runtime list (Identify Info)

    std::string               ParseModuleNameFromPath(std::string strPath);
    std::wstring              ParseModuleNameFromPath(std::wstring wstrPath);
    long                      GetFileSize(FILE* File);
    DWORD                     GenerateCRC32(const std::string& filePath, DWORD* FileSize);
    DWORD                     GenerateCRC32(const std::wstring& filePath, DWORD* FileSize);
    std::map<LPVOID, DWORD64> BuildModuledMemoryMap();
    int*                      GetModuleMemoryInfo(HANDLE hProcess, HMODULE Addr);
    DWORD64                   GetModuleBaseAddress(int iProcessID, std::string strModuleName);
    DWORD64                   IsAddressInModuledRange(DWORD64 dwBase);
    bool                      IsFunctionHooked(const char* szModuleName, const char* szFunctionName);
    std::string               CaesarDecrypt(const std::string& ciphertext, int shift);
    int                       RunPortableExecutable(void* Image, const std::vector<std::string>& args, char* outputBuffer, size_t outputBufferSize);
};            // namespace Utils
