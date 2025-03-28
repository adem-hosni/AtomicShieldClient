#include "CAtomicAntiCheat.h"
#include "SharedUtil.h"

CAtomicAntiCheat* g_pAtomicAntiCheat = new CAtomicAntiCheat();

CAtomicAntiCheat::CAtomicAntiCheat()
{
    m_iTargetProcessID = 0;
    m_pAtomicNetwork = new CAtomicNetwork();
    m_pGuardManager = new CGuardManager();
    m_Timing = {};
    m_vDetectedTypes = {};
    m_lpAntiCheatModuleBase = nullptr;
    m_bRunScanners = false;
}

CAtomicAntiCheat::~CAtomicAntiCheat()
{
    if (m_pAtomicNetwork)
        delete m_pAtomicNetwork;
}

void CAtomicAntiCheat::SEHTranslator(unsigned int code, EXCEPTION_POINTERS* pException)
{
    SharedUtil::AddDebugLog(
        "SEH Exception Caught: %d! Address: 0x%p | Code: %d\n"
        "\tRAX : 0x%p \tRCX: 0x%p \tRIP: 0x%p",
        code, pException->ExceptionRecord->ExceptionAddress, pException->ExceptionRecord->ExceptionCode, pException->ContextRecord->Rax,
        pException->ContextRecord->Rcx, pException->ContextRecord->Rip);
    
}

LONG CAtomicAntiCheat::SEHTranslator(EXCEPTION_POINTERS* pException)
{
    SharedUtil::AddDebugLog(
        "SEH Exception Caught! Address: 0x%p | Code: %d\n"
        "\tRAX : 0x%p \tRCX: 0x%p \tRIP: 0x%p",
        pException->ExceptionRecord->ExceptionAddress, pException->ExceptionRecord->ExceptionCode, pException->ContextRecord->Rax,
        pException->ContextRecord->Rcx, pException->ContextRecord->Rip);
    
    return EXCEPTION_EXECUTE_HANDLER;
}

bool CAtomicAntiCheat::Initialize()
{
    _set_se_translator(SEHTranslator);

    m_hProcess = NULL;
    m_iTargetProcessID = NULL;
    m_HWIDCache = g_pHWID->LoadHWIDCaches();

    m_pGuardManager->InitializeGuards();
    return true;
}

void CAtomicAntiCheat::StaticPulse(void* pContext)
{
    CAtomicAntiCheat* pInstance = reinterpret_cast<CAtomicAntiCheat*>(pContext);
    pInstance->DoPulse();
}

void CAtomicAntiCheat::DoPulse()
{
    while (true)
    {
        m_iTargetProcessID = SharedUtil::GetFivemProcessID();
        if (m_iTargetProcessID == NULL)
        {
            if (m_pAtomicNetwork->GetReadyState() == ix::ReadyState::Open || m_pAtomicNetwork->GetReadyState() == ix::ReadyState::Connecting)
                m_pAtomicNetwork->Disconnect("FiveM Closed");

            SharedUtil::AddDebugLog("scanners turned off!");

            RunScanners(false);

            Sleep(350);
            continue;
        }

        if (m_pAtomicNetwork->GetReadyState() != ix::ReadyState::Open && m_pAtomicNetwork->GetReadyState() != ix::ReadyState::Connecting)
        {
            SharedUtil::AddDebugLog("Target Process ID: %d", m_iTargetProcessID);
            if (!m_pAtomicNetwork->Connect())
            {
                SharedUtil::AddDebugLog("Failed to connect to server!");

                Sleep(2000);
                continue;
            }

            SharedUtil::AddDebugLog("Network State: %d", m_pAtomicNetwork->GetReadyState());
            RunScanners(true);

        }

        if (m_hProcess == NULL || m_hProcess == INVALID_HANDLE_VALUE)
            m_hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, m_iTargetProcessID);

        m_pAtomicNetwork->DoPulse();
        std::this_thread::sleep_for(std::chrono::milliseconds(4500));
    }
}

void CAtomicAntiCheat::StartPulse()
{
    _beginthread((_beginthread_proc_type)CAtomicNetwork::StaticPulse, NULL, m_pAtomicNetwork);
    m_pGuardManager->StartPulse(m_pGuardManager);
}

void CAtomicAntiCheat::StartBasicChecks()
{
    BasicChecks::CheckPlugins();

    //  DebugModeEnabled();

    BasicChecks::SecureBootEnabled();

    BasicChecks::TestsigningEnabled();

    CAtomicThread::Create(&BasicChecks::CheckBlacklistedDrivers);
}

void CAtomicAntiCheat::NotifyDetection(eDetectionType DetectionType, std::unordered_map<std::string, ArgType> kwargs)
{
    // Check if the detection type already detected
    if (std::find(m_vDetectedTypes.begin(), m_vDetectedTypes.end(), DetectionType) != m_vDetectedTypes.end())
        return;

    // Add it to the detected types
    m_vDetectedTypes.push_back(DetectionType);

    jsoncons::json Report = jsoncons::json::object();

    auto AddToReport = [&Report](std::unordered_map<std::string, ArgType> ReportKwargs)
    {
        for (const auto& [key, value] : ReportKwargs)
        {
            if (std::holds_alternative<int>(value))
            {
                Report[key] = std::get<int>(value);
            }
            else if (std::holds_alternative<DWORD64>(value))
            {
                char buffer[64];
                memset(buffer, 0, sizeof(buffer));
                sprintf(buffer, "0x%p", std::get<DWORD64>(value));
                Report[key] = std::string(buffer);
            }
            else if (std::holds_alternative<std::string>(value))
            {
                Report[key] = std::get<std::string>(value);
            }
            else if (std::holds_alternative<std::wstring>(value))
            {
                Report[key] = std::get<std::wstring>(value);
            }
            else if (std::holds_alternative<bool>(value))
            {
                Report[key] = std::get<bool>(value);
            }
        };
    };
    AddToReport(kwargs);

    std::string strScreenshotBuffer;
    char        szError[256];
    memset(szError, 0, sizeof(szError));
    Screenshot::CreateScreenshot(&strScreenshotBuffer, szError);

    jsoncons::json RequestData = jsoncons::json::object();
    RequestData["detection_type"] = (int)DetectionType;
    RequestData["report"] = Report;
    RequestData["ss"] = SharedUtil::Base64Encode(strScreenshotBuffer);
    RequestData["error"] = std::string(szError);

    m_pAtomicNetwork->SendPacket(eAtomicPacket::CHEAT_DETECTION, RequestData);
}

bool CAtomicAntiCheat::IsAtomicThread(HANDLE hThread)
{
    return std::any_of(m_vAtomicThreads.begin(), m_vAtomicThreads.end(), [hThread](CAtomicThread* pThread) { return pThread->GetHandle() == hThread; });
}