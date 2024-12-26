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
    std::string basePath = SharedUtil::GetKnownDirectory(FOLDERID_LocalAppData) + "\\FiveM\\FiveM.app\\plugins";
    for (const auto& entry : std::filesystem::directory_iterator(basePath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".dll")
        {
            std::string fileName = entry.path().filename().string();
            if (fileName.find("d3d9") != std::string::npos || fileName.find("d3d10") != std::string::npos)
            {
                g_pSafeAntiCheat->NotifyDetection(eDetectionType::DLL_FOUND, nullptr);
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

void CSafeAntiCheat::TestsigningEnabled()
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

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))            // use a pipe to read output of bcdedit command
    {
        SharedUtil::AddDebugLog("CreatePipe failed @ Services::IsMachineAllowingSelfSignedDrivers: %d", GetLastError());
        return;
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWritePipe;

    std::string bcdedit_location = "Windows\\System32\\bcdedit.exe";
    std::string fullpath_bcdedit = (volumeName.c_str() + bcdedit_location);

    if (!CreateProcessA(fullpath_bcdedit.c_str(), NULL, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        SharedUtil::AddDebugLog("CreateProcess failed @ Services::IsMachineAllowingSelfSignedDrivers: %d", GetLastError());
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return;
    }

    //..wait for the process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hWritePipe);
    CloseHandle(pi.hThread);

    if (!ReadFile(hReadPipe, szOutput, 1024 - 1, &bytesRead, NULL))            // now read our pipe
    {
        SharedUtil::AddDebugLog("ReadFile failed @ Services::IsMachineAllowingSelfSignedDrivers: %d", GetLastError());
        CloseHandle(hReadPipe);
        return;
    }

    CloseHandle(hReadPipe);

    szOutput[bytesRead] = '\0';

    if (strstr(szOutput, "The boot configuration data store could not be opened") != NULL)
    {
        SharedUtil::AddDebugLog("Failed to run bcdedit @ IsMachineAllowingSelfSignedDrivers. Please make sure program is run as administrator\n");
        return;
    }

    char* token = strtok(szOutput, "\r\n");

    while (token != NULL)
    {
        if (strstr(token, "testsigning") != NULL && strstr(token, "Yes") != NULL)
        {
            SMemoryDetectionReport report = {0};
            g_pSafeAntiCheat->NotifyDetection(eDetectionType::TEST_SIGNING_ENABLED, &report);
            SharedUtil::AddDebugLog("Test Signing is Enabled\n");
        }

        token = strtok(NULL, "\r\n");
    }
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
    HRESULT        hr;
    IWbemLocator*  pLoc = nullptr;
    IWbemServices* pSvc = nullptr;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Failed to initialize COM library.");
        return;
    }

    // Create the WMI locator object
    hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Failed to create IWbemLocator object.");
        CoUninitialize();
        return;
    }

    // Connect to the WMI namespace
    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Could not connect to WMI namespace.");
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Set proxy blanket for security
    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Could not set proxy blanket.");
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    // Query Secure Boot status
    IEnumWbemClassObject* pEnumerator = nullptr;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT SecureBootEnabled FROM Win32_ComputerSystem"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                         nullptr, &pEnumerator);
    if (FAILED(hr))
    {
        SharedUtil::AddDebugLog("Query for SecureBootEnabled failed.");
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    IWbemClassObject* pClassObj = nullptr;
    ULONG             uReturn = 0;

    // Check if we have data
    hr = pEnumerator->Next(WBEM_INFINITE, 1, &pClassObj, &uReturn);
    if (FAILED(hr) || uReturn == 0)
    {
        SharedUtil::AddDebugLog("Failed to retrieve SecureBootEnabled information. 0x%x", GetLastError());
        pEnumerator->Release();
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return;
    }

    VARIANT vtProp;
    hr = pClassObj->Get(L"SecureBootEnabled", 0, &vtProp, nullptr, nullptr);
    if (SUCCEEDED(hr))
    {
        bool secureBootEnabled = vtProp.boolVal == VARIANT_TRUE;
        if (!secureBootEnabled)
        {
            g_pSafeAntiCheat->NotifyDetection(eDetectionType::SECURE_BOOT_DISABLED, nullptr);
            SharedUtil::AddDebugLog("Secure Boot is disabled.");
        }
        else
        {
            SharedUtil::AddDebugLog("Secure Boot is enabled.");
        }
        VariantClear(&vtProp);
    }
    else
    {
        SharedUtil::AddDebugLog("Failed to retrieve SecureBootEnabled property.");
    }

    pClassObj->Release();
    pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
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

    DebugModeEnabled();

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