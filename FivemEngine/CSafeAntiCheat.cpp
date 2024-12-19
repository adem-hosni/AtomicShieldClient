#include "CSafeAntiCheat.h"
#include "SharedUtil.h"
#include <ctime>

CSafeAntiCheat* g_pSafeAntiCheat = new CSafeAntiCheat();

CSafeAntiCheat::CSafeAntiCheat()
{
    m_iTargetProcessID = 0;
    m_pSafeNetwork = new CSafeNetwork();
    m_pGuardManager = new CGuardManager();
    m_Timing = {};
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

void CSafeAntiCheat::NotifyDetection(eDetectionType DetectionType, SMemoryDetectionReport* pDetectionInfo)
{
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