#include "CSafeAntiCheat.h"
#include "SharedUtil.h"

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

void CSafeAntiCheat::DoPulse()
{
    while (true)
    {
        m_pSafeNetwork->DoPulse();

        long long llCurrentTime = time(NULL);
        STiming&  Timing = g_pSafeAntiCheat->GetTiming();

        if (!m_hProcess)
            m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_iTargetProcessID);

        m_pGuardManager->GetMemoryGuard()->DoPulse();
    }
}

void CSafeAntiCheat::StartPulse()
{
    _beginthread((_beginthread_proc_type)CSafeNetwork::StaticPulse, NULL, m_pSafeNetwork);
    m_pGuardManager->StartPulse(m_pGuardManager);
}

void CSafeAntiCheat::StartBasicChecks()
{
    BasicChecks::CheckPlugins();

    //  DebugModeEnabled();

    BasicChecks::SecureBootEnabled();

    BasicChecks::TestsigningEnabled();
}

void CSafeAntiCheat::NotifyDetection(eDetectionType DetectionType, std::unordered_map<std::string, ArgType> kwargs)
{
    // Check if the detection type already detected
    if (std::find(m_vDetectedTypes.begin(), m_vDetectedTypes.end(), DetectionType) != m_vDetectedTypes.end())
        return;

    // Add it to the detected types
    m_vDetectedTypes.push_back(DetectionType);

    jsoncons::json Report = jsoncons::json::object();

    auto AddToReport = [&Report](std::unordered_map<std::string, ArgType> ReportKwargs)
    {
        jsoncons::json j;

        for (const auto& [key, value] : ReportKwargs)
        {
            if (std::holds_alternative<int>(value))
            {
                j[key] = std::get<int>(value);
            }
            else if (std::holds_alternative<std::string>(value))
            {
                j[key] = std::get<std::string>(value);
            }
        };
    };
    AddToReport(kwargs);
        
    jsoncons::json RequestData = jsoncons::json::object();
    RequestData["detection_type"] = (int)DetectionType;
    RequestData["report"] = Report;

    m_pSafeNetwork->SendPacket(eSafePacketID::CHEAT_DETECTION, RequestData);
}

bool CSafeAntiCheat::IsAtomicThread(HANDLE hThread)
{
    return std::any_of(m_vAtomicThreads.begin(), m_vAtomicThreads.end(), [hThread](CAtomicThread* pThread) { return pThread->GetHandle() == hThread; });
}