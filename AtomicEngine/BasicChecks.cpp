#include "StdInc.h"
#include <ctime>
#include <filesystem>
#include <WbemCli.h>
#include <comutil.h>

void BasicChecks::CheckPlugins()
{
    std::string basePath = SharedUtil::GetKnownDirectory(FOLDERID_LocalAppData) + "\\FiveM\\FiveM.app\\plugins";
    for (const auto& entry : std::filesystem::directory_iterator(basePath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".dll")
        {
            std::string fileName = entry.path().filename().string();
            if (fileName.find("d3d9") != std::string::npos || fileName.find("d3d10") != std::string::npos)
            {
                SharedUtil::AddDebugLog("Found Dll ", fileName);
                g_pAtomicAntiCheat->NotifyDetection(eDetectionType::DLL_FOUND, {{"plugin", fileName}});
            }
        }
    }
}

std::string GetWindowsDrive()
{
    CHAR  volumePath[MAX_PATH];
    DWORD charCount;

    charCount = GetWindowsDirectoryA(volumePath, MAX_PATH);
    if (charCount == 0)
    {
        //     Logger::logf("UltimateAnticheat.log", Err, "Failed to retrieve Windows directory path @ Services::GetWindowsPath: %d", GetLastError());
        return "";
    }

    CHAR volumeName[MAX_PATH];
    if (!GetVolumePathNameA(volumePath, volumeName, MAX_PATH))
    {
        //     Logger::logf("UltimateAnticheat.log", Err, "Failed to retrieve volume path name @ Services::GetWindowsPath: %d", GetLastError());
        return "";
    }

    return volumeName;
}

inline bool CheckTestSign_Type1()
{
    SYSTEM_CODEINTEGRITY_INFORMATION sci = {0};
    sci.Length = sizeof(sci);

    auto dwcbSz = 0UL;
    auto ntStat = NtQuerySystemInformation(SystemCodeIntegrityInformation, &sci, sizeof(sci), &dwcbSz);
    if (!NT_SUCCESS(ntStat) || dwcbSz != sizeof(sci))
        return false;

    auto bTestsigningEnabled = !!(sci.CodeIntegrityOptions & CODEINTEGRITY_OPTION_TESTSIGN);
    return bTestsigningEnabled;
}

inline bool CheckTestSign_Type2()
{
    bool  bRet = false;
    char  RegKey[_MAX_PATH] = {0};
    DWORD BufSize = _MAX_PATH;
    DWORD dataType = REG_DWORD;

    HKEY hKey;
    long lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ("SYSTEM\\CurrentControlSet\\Control\\CI"), NULL, KEY_QUERY_VALUE, &hKey);
    if (lError == ERROR_SUCCESS)
    {
        long lVal = RegQueryValueExA(hKey, ("DebugFlags"), NULL, &dataType, (LPBYTE)&RegKey, &BufSize);
        if (lVal == ERROR_SUCCESS)
        {
            if (!strcmp(RegKey, ("1")))
                bRet = true;
        }
        RegCloseKey(hKey);
    }
    return bRet;
}

struct Service
{
    std::wstring displayName;
    std::wstring serviceName;
    DWORD        pid;
    bool         isRunning;
};

bool ContainsWStringInsensitive(const std::string& str, const std::string& substr)
{
    auto it = search(str.begin(), str.end(), substr.begin(), substr.end(), [](char ch1, char ch2) { return towlower(ch1) == towlower(ch2); });
    return it != str.end();
}

bool GetLoadedDrivers(std::vector<std::string>& vDriverPaths, std::vector<std::string>& vFoundBlacklistedDrivers,
                      const std::vector<std::string>& blacklistedDrivers)
{
    DWORD   cbNeeded;
    HMODULE drivers[1024];
    DWORD   numDrivers;

    if (!EnumDeviceDrivers((LPVOID*)drivers, sizeof(drivers), &cbNeeded))
    {
        SharedUtil::AddDebugLog("Failed to enumerate device drivers. Error: ", GetLastError());
        return false;
    }

    numDrivers = cbNeeded / sizeof(HMODULE);

    for (DWORD i = 0; i < numDrivers; i++)
    {
        TCHAR szDriverPath[MAX_PATH];

        if (GetDeviceDriverFileName(drivers[i], szDriverPath, MAX_PATH))
        {
            vDriverPaths.push_back(szDriverPath);

            for (const std::string& blacklisted : blacklistedDrivers)
            {
                if (ContainsWStringInsensitive(szDriverPath, blacklisted))
                {
                    vFoundBlacklistedDrivers.push_back(szDriverPath);
                }
            }
        }
        else
        {
            SharedUtil::AddDebugLog("Failed to get driver information. Error: ", GetLastError());
        }
    }

    return !blacklistedDrivers.empty();
}

void BasicChecks::CheckBlacklistedDrivers()
{
    std::vector<std::string> vBlacklistedDrivers = {"ntguard.sys",
                                                   "BEDaisy.sys",
                                                   "Gdrv.sys",
                                                   "AsIO.sys",
                                                   "AsUpIO.sys",
                                                   "CPUID.sys",
                                                   "ENE.sys",
                                                   "iqvw64e.sys",
                                                   "hxctl.sys",
                                                   "kprocesshacker.sys",
                                                   "kprocesshacker2.sys",
                                                   "EIO64.sys",
                                                   "IOMap64.sys",
                                                   "ATSZIO64.sys",
                                                   "atillk64.sys",
                                                   "aswVmm.sys",
                                                   "BS_Flash64.sys",
                                                   "Capcom.sys",
                                                   "cpuz141.sys",
                                                   "WinRing0x64.sys",
                                                   "FairplayKD.sys",
                                                   "pgldqpoc.sys",
                                                   "HwOs2Ec10x64.sys",
                                                   "Phymemx64.sys",
                                                   "Monitor_win10_x64.sys",
                                                   "driver.sys",
                                                   "lha.sys",
                                                   "Mslo64.sys",
                                                   "NTIOLib_x64.sys",
                                                   "pcdsrvc_x64.pkms",
                                                   "HWiNFO64A.sys",
                                                   "rzpnk.sys",
                                                   "magdrvamd64.sys",
                                                   "speedfan.sys",
                                                   "zam64.sys",
                                                   "DBK64.sys"};

    std::vector<std::string> vDriverPaths;
    std::vector<std::string> vFoundBlacklistedDrivers;

    if (GetLoadedDrivers(vDriverPaths, vFoundBlacklistedDrivers, vBlacklistedDrivers))
    {
        std::unordered_map<std::string, ArgType> params;
        for (auto& driver : vFoundBlacklistedDrivers)
        {
            params.insert_or_assign("driver_name", driver);
        }

        g_pAtomicAntiCheat->NotifyDetection(eDetectionType::BLACKLISTED_DRIVER_LOADED, params);
    }
}

inline bool CheckTestSign_Type3()
{
    bool  bRet = false;
    char  RegKey[_MAX_PATH] = {0};
    DWORD BufSize = _MAX_PATH;
    DWORD dataType = REG_SZ;

    HKEY hKey;
    long lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ("SYSTEM\\CurrentControlSet\\Control"), NULL, KEY_QUERY_VALUE, &hKey);
    if (lError == ERROR_SUCCESS)
    {
        long lVal = RegQueryValueExA(hKey, ("SystemStartOptions"), NULL, &dataType, (LPBYTE)&RegKey, &BufSize);
        if (lVal == ERROR_SUCCESS)
        {
            if (strstr(RegKey, ("TESTSIGNING")))
                bRet = true;
        }
        RegCloseKey(hKey);
    }
    return bRet;
}

inline bool CheckTestSign_Type4()
{
    bool  bRet = false;
    char  RegKey[_MAX_PATH] = {0};
    DWORD BufSize = _MAX_PATH;
    DWORD dataType = REG_SZ;

    HKEY hKey;
    long lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ("SYSTEM\\CurrentControlSet\\Control"), NULL, KEY_QUERY_VALUE, &hKey);
    if (lError == ERROR_SUCCESS)
    {
        long lVal = RegQueryValueExA(hKey, ("SystemStartOptions"), NULL, &dataType, (LPBYTE)&RegKey, &BufSize);
        if (lVal == ERROR_SUCCESS)
        {
            if (strstr(RegKey, ("DISABLE_INTEGRITY_CHECKS")))
                bRet = true;
        }
        RegCloseKey(hKey);
    }
    return bRet;
}

inline bool CheckTestSign_Type5()
{
    HKEY hTestKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, ("BCD00000000\\Objects"), 0, KEY_READ, &hTestKey) != ERROR_SUCCESS)
        return false;

    char     achKey[255];
    DWORD    cbName;
    char     achClass[MAX_PATH] = "";
    DWORD    cchClassName = MAX_PATH;
    DWORD    cSubKeys = 0;
    DWORD    cbMaxSubKey;
    DWORD    cchMaxClass;
    DWORD    cValues;
    DWORD    cchMaxValue;
    DWORD    cbMaxValueData;
    DWORD    cbSecurityDescriptor;
    FILETIME ftLastWriteTime;

    bool bRet = false;

    DWORD dwReturn[1000];
    DWORD dwBufSize = sizeof(dwReturn);

    auto dwApiRetCode = RegQueryInfoKeyA(hTestKey, achClass, &cchClassName, NULL, &cSubKeys, &cbMaxSubKey, &cchMaxClass, &cValues, &cchMaxValue,
                                         &cbMaxValueData, &cbSecurityDescriptor, &ftLastWriteTime);

    if (cSubKeys)
    {
        for (DWORD i = 0; i < cSubKeys; i++)
        {
            cbName = 255;
            dwApiRetCode = RegEnumKeyExA(hTestKey, i, achKey, &cbName, NULL, NULL, NULL, &ftLastWriteTime);
            if (dwApiRetCode == ERROR_SUCCESS)
            {
                char szNewWay[4096];
                sprintf(szNewWay, ("BCD00000000\\Objects\\%s\\Elements\\16000049"), achKey);

                HKEY hnewKey;
                long lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szNewWay, NULL, KEY_QUERY_VALUE, &hnewKey);
                if (lError == ERROR_SUCCESS)
                {
                    long lVal = RegQueryValueExA(hnewKey, ("Element"), NULL, 0, (LPBYTE)dwReturn, &dwBufSize);
                    if (lVal == ERROR_SUCCESS)
                    {
                        if (dwReturn[0] == 1UL)
                            bRet = true;
                    }
                    RegCloseKey(hnewKey);
                }
            }
        }
    }

    RegCloseKey(hTestKey);
    return bRet;
}

bool CheckTestSign_Type6()
{
    bool  bRet = false;
    BYTE  Result;
    DWORD BufSize = sizeof(Result);
    DWORD dataType = REG_BINARY;

    HKEY hKey;
    long lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ("SOFTWARE\\Microsoft\\Driver Signing"), NULL, KEY_QUERY_VALUE, &hKey);
    if (lError == ERROR_SUCCESS)
    {
        long lVal = RegQueryValueExA(hKey, ("Policy"), NULL, &dataType, &Result, &BufSize);
        if (lVal == ERROR_SUCCESS)
        {
            if (Result == 0x02)
                bRet = true;
        }
        RegCloseKey(hKey);
    }
    return bRet;
}

bool IsTestSignEnabled()
{
    if (CheckTestSign_Type1())
        return true;

    if (CheckTestSign_Type2())
        return true;

    if (CheckTestSign_Type3())
        return true;

    if (CheckTestSign_Type4())
        return true;

    if (CheckTestSign_Type5())
        return true;

    if (CheckTestSign_Type6())
        return true;

    return false;
}

void BasicChecks::TestsigningEnabled()
{
    if (IsTestSignEnabled())
    {
        g_pAtomicAntiCheat->NotifyDetection(eDetectionType::TEST_SIGNING_ENABLED);
    }
}

void BasicChecks::DebugModeEnabled()
{
    HANDLE              hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA        si;
    PROCESS_INFORMATION pi;
    char                szOutput[1024];
    DWORD               bytesRead;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    std::string volumeName = GetWindowsDrive();

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        SharedUtil::AddDebugLog("CreatePipe failed @BasicChecks::IsMachineAllowingSelfSignedDrivers: %d", GetLastError());
        return;
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;

    std::string bcdedit_location = "Windows\\System32\\bcdedit.exe";
    std::string fullpath_bcdedit = (volumeName + bcdedit_location);

    if (!CreateProcessA(fullpath_bcdedit.c_str(), NULL, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        SharedUtil::AddDebugLog("CreateProcess failed @BasicChecks::IsDebugModeEnabled: %d", GetLastError());
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hWritePipe);
    CloseHandle(pi.hThread);

    if (!ReadFile(hReadPipe, szOutput, 1024 - 1, &bytesRead, NULL))            // now read our pipe
    {
        SharedUtil::AddDebugLog("ReadFile failed @ BasicChecks::IsDebugModeEnabled: %d", GetLastError());
        CloseHandle(hReadPipe);
        return;
    }

    CloseHandle(hReadPipe);

    szOutput[bytesRead] = '\0';

    if (strstr(szOutput, "The boot configuration data store could not be opened") != NULL)
    {
        SharedUtil::AddDebugLog("Failed to run bcdedit @IsMachineAllowingSelfSignedDrivers. Please make sure program is run as administrator");
        return;
    }

    char* token = strtok(szOutput, "\r\n");            // split based on new line

    while (token != NULL)            // Iterate through tokens, both "yes" and "debug" on same line = debug mode
    {
        if (strstr(token, "debug") != NULL && strstr(token, "Yes") != NULL)
        {
            g_pAtomicAntiCheat->NotifyDetection(eDetectionType::DEBUG_MODE_ENABLED);
        }

        token = strtok(NULL, "\r\n");
    }
}

void BasicChecks::SecureBootEnabled()
{
    HKEY        hKey;
    LONG        lResult;
    DWORD       dwSize = sizeof(DWORD);
    DWORD       dwValue = 0;
    const char* registryPath = "SYSTEM\\CurrentControlSet\\Control\\SecureBoot\\State";            // optionally xor this
    const char* valueName = "UEFISecureBootEnabled";

    lResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, registryPath, 0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS)
    {
        return;
    }

    lResult = RegQueryValueExA(hKey, valueName, NULL, NULL, (LPBYTE)&dwValue, &dwSize);

    if (lResult != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }

    if (dwValue == 1)
    {
        RegCloseKey(hKey);
    }
    else
    {
        RegCloseKey(hKey);
        g_pAtomicAntiCheat->NotifyDetection(eDetectionType::SECURE_BOOT_DISABLED);
    }
}