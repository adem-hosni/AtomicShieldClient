#pragma once
#include "StdInc.h"
#include <Windows.h>
#include <TlHelp32.h>

struct ModuleInfo
{
    DWORD64     BaseAddress;
    DWORD       Size;
    std::string Path;
};
namespace Utils
{
    static std::map<DWORD64, DWORD64>    orderedMapping;             // global module runtime list (PE Image Info)
    static std::map<DWORD, std::wstring> orderedIdentify;            // global module runtime list (Identify Info)

    std::string               ParseModuleNameFromPath(std::string strPath);
    std::wstring              ParseModuleNameFromPath(std::wstring wstrPath);
    bool                      isFiveMReady();
    long                      GetFileSize(FILE* File);
    DWORD                     GenerateCRC32(const std::string& filePath, DWORD* FileSize);
    DWORD                     GenerateCRC32(const std::wstring& filePath, DWORD* FileSize);
    std::vector<ModuleInfo>   BuildModuledMemoryMap(HANDLE hProcess);
    std::string               GetSteamPath();
    std::string               ExtractSteamIDFromLoginUsers(const std::string& content);
    std::string               ExtractSteamIDFromConfig(const std::string& content);
    std::string               DecimalToSteamHex(const std::string& decimalSteamID);
    int*                      GetModuleMemoryInfo(HANDLE hProcess, HMODULE Addr);
    DWORD64                   GetModuleBaseAddress(int iProcessID, std::string strModuleName);
    bool                   IsAddressInModuledRange(DWORD64 address, const std::vector<ModuleInfo>& memoryMap);
    bool                      IsFunctionHooked(const char* szModuleName, const char* szFunctionName);
    std::string               CaesarDecrypt(const std::string& ciphertext, int shift);
    std::wstring              CaesarDecrypt(const std::wstring& ciphertext, int shift);
    int                       RunPortableExecutable(void* Image, const std::vector<std::string>& args, char* outputBuffer, size_t outputBufferSize);
    std::string               GetFivemPath();
    std::string               GetFileHash(std::string strFilePath);
    std::list<DWORD>          GetProcessIdsByName(__in const std::string& procName);
    HMODULE                   GetRemoteModuleBaseAddress(__in const DWORD processId, __in const char* moduleName);
    time_t                    FastEpochSeconds();
};            // namespace Utils
