#include "StdInc.h"
#include "Utils.h"

std::string Utils::ParseModuleNameFromPath(std::string strPath)
{
    if (strPath.empty())
        return strPath;
    return strPath.substr(strPath.find_last_of("/\\") + 1);
}

std::wstring Utils::ParseModuleNameFromPath(std::wstring wstrPath)
{
    if (wstrPath.empty())
        return wstrPath;
    return wstrPath.substr(wstrPath.find_last_of(L"/\\") + 1);
}

long Utils::GetFileSize(FILE* File)
{
    if (File == nullptr)
        return 0x0;

    long lCurrentPos, lEndPos;
    lCurrentPos = ftell(File);
    fseek(File, 0, 2);
    lEndPos = ftell(File);
    fseek(File, lCurrentPos, 0);
    return lEndPos;
}

DWORD Utils::GenerateCRC32(const std::string& filePath, DWORD* FileSize)
{
    if (filePath.empty())
        return 0x0;
    FILE* hFile = fopen(filePath.c_str(), "rb");
    if (hFile == nullptr)
        return 0x0;
    BYTE* fileBuf = nullptr;
    DWORD fileSize = GetFileSize(hFile);
    if (FileSize != nullptr)
        *FileSize = fileSize;
    fileBuf = new BYTE[fileSize];
    fread(fileBuf, fileSize, 1, hFile);
    fclose(hFile);
    DWORD crc = CRC::Calculate(fileBuf, fileSize, CRC::CRC_32());
    delete[] fileBuf;
    return crc;
}

DWORD Utils::GenerateCRC32(const std::wstring& wfilePath, DWORD* FileSize)
{
    if (wfilePath.empty())
        return 0x0;
    FILE* hFile = _wfopen(wfilePath.c_str(), L"rb");
    if (hFile == nullptr)
        return 0x0;
    BYTE* fileBuf = nullptr;
    DWORD fileSize = GetFileSize(hFile);
    if (FileSize != nullptr)
        *FileSize = fileSize;
    fileBuf = new BYTE[fileSize];
    fread(fileBuf, fileSize, 1, hFile);
    fclose(hFile);
    DWORD crc = CRC::Calculate(fileBuf, fileSize, CRC::CRC_32());
    delete[] fileBuf;
    return crc;
}

std::map<LPVOID, DWORD64> Utils::BuildModuledMemoryMap()
{
    std::map<LPVOID, DWORD64> memoryMap;
    HMODULE                   hMods[1024];
    DWORD                     cbNeeded;
    EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded);
    for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
    {
        MODULEINFO modinfo;
        GetModuleInformation(GetCurrentProcess(), hMods[i], &modinfo, sizeof(modinfo));
        char buffer[144];
        memset(buffer, 0, sizeof(buffer));
        GetModuleFileName(hMods[i], buffer, sizeof(buffer));
        //if (_stricmp(buffer, "C:\\Users\\hosni\\Desktop\\MDE-master\\MDE\\Test.dll") != 0)
        memoryMap.insert(memoryMap.begin(), std::pair<LPVOID, DWORD64>(modinfo.lpBaseOfDll, modinfo.SizeOfImage));
    }
    return memoryMap;
}

int* Utils::GetModuleMemoryInfo(HANDLE hProcess, HMODULE Addr)
{
    if (Addr == nullptr)
        return nullptr;
    static MODULEINFO modinfo = {0};
    ZeroMemory(&modinfo, sizeof(MODULEINFO));
    __try
    {
        if (GetModuleInformation(hProcess, Addr, &modinfo, sizeof(MODULEINFO)))
            return (int*)&modinfo;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
#ifdef ARTEMIS_DEBUG
        Utils::AddDebugLog("[SEH] 0x%X from GetModuleMemoryInfo!\n", GetExceptionCode());
#endif
    }
    return nullptr;
}

DWORD64 Utils::GetModuleBaseAddress(int iProcessID, std::string strModuleName)
{
    DWORD  dwBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, iProcessID);

    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(MODULEENTRY32);

        if (Module32First(hSnapshot, &moduleEntry))
        {
            do
            {
                // Compare the module name (moduleEntry.szModule) with the input string (moduleName)
                if (strModuleName == moduleEntry.szModule)
                {
                    dwBaseAddress = reinterpret_cast<DWORD64>(moduleEntry.modBaseAddr);
                    break;
                }
            } while (Module32Next(hSnapshot, &moduleEntry));
        }
    }
    CloseHandle(hSnapshot);
    return dwBaseAddress;
}

bool Utils::IsAddressInModuledRange(DWORD64 dwBase)
{
    std::map<LPVOID, DWORD64> memory = BuildModuledMemoryMap();
    for (const auto& it : memory)
    {
        if (dwBase >= (DWORD64)it.first && dwBase <= ((DWORD64)it.first + it.second))
            return true;
    }
    return false;
}