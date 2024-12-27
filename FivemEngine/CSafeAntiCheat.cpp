#include "CSafeAntiCheat.h"
#include "SharedUtil.h"
#include <ctime>
#include <filesystem>
#include <WbemCli.h>
#include <comutil.h>

CSafeAntiCheat* g_pSafeAntiCheat = new CSafeAntiCheat();

CSafeAntiCheat::CSafeAntiCheat()
{
    m_iTargetProcessID = 0;
    m_pSafeNetwork = new CSafeNetwork();
    m_pGuardManager = new CGuardManager();
    m_Timing = {};
    m_vDetectedTypes = {};
}

CSafeAntiCheat::~CSafeAntiCheat()
{
    if (m_pSafeNetwork)
        delete m_pSafeNetwork;
}

bool CSafeAntiCheat::Initialize()
{
    m_hProcess = GetCurrentProcess();
    m_iTargetProcessID = GetCurrentProcessId();
    m_HWIDCache = g_pHWID->LoadHWIDCaches();

    if (!m_pSafeNetwork->Connect())
    {
        MessageBox(0, "Failed to connect to the server", "Error", 0);
        return false;
    }

    m_pGuardManager->InitializeGuards();
    return true;
}

void CSafeAntiCheat::StaticPulse(void* pContext)
{
    CSafeAntiCheat* pInstance = reinterpret_cast<CSafeAntiCheat*>(pContext);
    pInstance->DoPulse();
}

void CSafeAntiCheat::CheckPlugins()
{
    SharedUtil::AddDebugLog("Plugins Checked");
    std::string basePath = SharedUtil::GetKnownDirectory(FOLDERID_LocalAppData) + "\\FiveM\\FiveM.app\\plugins";
    for (const auto& entry : std::filesystem::directory_iterator(basePath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".dll")
        {
            std::string fileName = entry.path().filename().string();
            if (fileName.find("d3d9") != std::string::npos || fileName.find("d3d10") != std::string::npos)
            {
                SharedUtil::AddDebugLog("Found Dll ",fileName);
              //  SMemoryDetectionReport report = {0};
             //   g_pSafeAntiCheat->NotifyDetection(eDetectionType::DLL_FOUND, &report);
            }
        }
    }
}

std::string CSafeAntiCheat::GetWindowsDrive()
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

void CSafeAntiCheat::TestsigningEnabled()
{
    SharedUtil::AddDebugLog("TestSigning Checked");

    if (IsTestSignEnabled())
    {
        SharedUtil::AddDebugLog("TestSigning Enabled");

        SMemoryDetectionReport report = {0};
        g_pSafeAntiCheat->NotifyDetection(eDetectionType::TEST_SIGNING_ENABLED, &report);
    }
    SharedUtil::AddDebugLog("TestSigning Disabled");


}

void CSafeAntiCheat::DebugModeEnabled()
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
        SharedUtil::AddDebugLog("CreatePipe failed @CSafeAntiCheat::IsMachineAllowingSelfSignedDrivers: %d", GetLastError());
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
        SharedUtil::AddDebugLog("CreateProcess failed @CSafeAntiCheat::IsDebugModeEnabled: %d", GetLastError());
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hWritePipe);
    CloseHandle(pi.hThread);

    if (!ReadFile(hReadPipe, szOutput, 1024 - 1, &bytesRead, NULL))            // now read our pipe
    {
        SharedUtil::AddDebugLog("ReadFile failed @ CSafeAntiCheat::IsDebugModeEnabled: %d", GetLastError());
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
            g_pSafeAntiCheat->NotifyDetection(eDetectionType::DEBUG_MODE_ENABLED, nullptr);
        }

        token = strtok(NULL, "\r\n");
    }
}

void CSafeAntiCheat::SecureBootEnabled()
{
    SharedUtil::AddDebugLog("SecureBoot Checked");

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
        
      //  Logger::logf("UltimateAnticheat.log", Warning, "RegCloseKey failed with error: %d @ Services::IsSecureBootEnabled_RegKey\n", lResult);
        RegCloseKey(hKey);
        return;
    }

    if (dwValue == 1)
    {
        RegCloseKey(hKey);
        SharedUtil::AddDebugLog("SecureBoot Enabled");

    }
    else
    {
        RegCloseKey(hKey);
        SharedUtil::AddDebugLog("SecureBoot Disabled");
        SMemoryDetectionReport report = {0};
        g_pSafeAntiCheat->NotifyDetection(eDetectionType::SECURE_BOOT_DISABLED, &report);
    }

}
void CSafeAntiCheat::DoPulse()
{
    while (true)
    {
        m_pSafeNetwork->DoPulse();

        long long llCurrentTime = time(NULL);
        STiming&  Timing = g_pSafeAntiCheat->GetTiming();

        if (!m_hProcess)
            m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_iTargetProcessID);

        if (!g_pMemoryScanner->IsAttached())
            g_pMemoryScanner->Attach(m_hProcess);

        m_pGuardManager->GetMemoryGuard()->DoPulse();

        if (llCurrentTime - Timing.llLastMemoryScan > GAME_MEMORY_SCAN_INTERVAL)
        {
            // g_pMemoryScanner->ScanStrings(pSafeNetwork->GetSignatures());

            std::vector<std::string> vSignatures = g_pMemoryScanner->GetDetectedSignatures();
            unsigned int             uiScanResult = vSignatures.size();

            // New Signature Found ?
            if (uiScanResult != g_pMemoryScanner->GetLatestScanResult())
            {
                g_pMemoryScanner->UpdateLatestScanResult(uiScanResult);
                jsoncons::json RequestData = jsoncons::json::object();
                RequestData["signatures"] = vSignatures;
                m_pSafeNetwork->SendPacket(eSafePacketID::MALICIOUS_SIGNATURE, RequestData);
            }
            Timing.llLastMemoryScan = llCurrentTime;
        }
    }
}

void CSafeAntiCheat::StartPulse()
{
    _beginthread((_beginthread_proc_type)CSafeNetwork::StaticPulse, NULL, m_pSafeNetwork);
    m_pGuardManager->StartPulse(m_pGuardManager);
}

void CSafeAntiCheat::StartBasicChecks()
{
    CheckPlugins();

  //  DebugModeEnabled();

    SecureBootEnabled();

    TestsigningEnabled();
}

void CSafeAntiCheat::NotifyDetection(eDetectionType DetectionType, SMemoryDetectionReport* pDetectionInfo)
{
    // Check if the detection type already detected
    if (std::find(m_vDetectedTypes.begin(), m_vDetectedTypes.end(), DetectionType) != m_vDetectedTypes.end())
        return;
    // Add it to the detected types
    m_vDetectedTypes.push_back(DetectionType);

    jsoncons::json DetectionReport = jsoncons::json::object();
    DetectionReport["allocated_base"] = (DWORD64)pDetectionInfo->AllocatedBase;
    DetectionReport["allocated_protect"] = (DWORD64)pDetectionInfo->AllocatedProtect;
    DetectionReport["region_size"] = (DWORD64)pDetectionInfo->RegionSize;
    DetectionReport["base_address"] = (DWORD64)pDetectionInfo->BaseAddress;

    jsoncons::json RequestData = jsoncons::json::object();
    RequestData["detection_type"] = (int)DetectionType;
    RequestData["memory_report"] = DetectionReport;

    m_pSafeNetwork->SendPacket(eSafePacketID::CHEAT_DETECTION, RequestData);
}

bool CSafeAntiCheat::IsAtomicThread(HANDLE hThread)
{
    return std::any_of(m_vAtomicThreads.begin(), m_vAtomicThreads.end(), [hThread](CAtomicThread* pThread) { return pThread->GetHandle() == hThread; });
}