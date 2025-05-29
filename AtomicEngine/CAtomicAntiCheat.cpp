#include "CAtomicAntiCheat.h"
#include "SharedUtil.h"

CAtomicAntiCheat* g_pAtomicAntiCheat = new CAtomicAntiCheat();

CAtomicAntiCheat::CAtomicAntiCheat()
{
    m_iTargetProcessID = 0;
    m_pAtomicNetwork = new CAtomicNetwork();
    m_pGuardManager = new CGuardManager();
    m_vDetectedTypes = {};
    m_lpAntiCheatModuleBase = nullptr;
    m_bRunScanners = true;
    m_bAlive = true;
}

CAtomicAntiCheat::~CAtomicAntiCheat()
{
    if (m_pAtomicNetwork)
        delete m_pAtomicNetwork;
}

LONG CAtomicAntiCheat::SEHTranslator(EXCEPTION_POINTERS* pException)
{
    if (!pException || !pException->ExceptionRecord || !pException->ContextRecord)
        return EXCEPTION_EXECUTE_HANDLER;

    SharedUtil::AddDebugLog(
        "SEH Exception Caught! Address: 0x%p | Code: 0x%08X\n"
        "\tRAX: 0x%p \tRCX: 0x%p \tRIP: 0x%p",
        pException->ExceptionRecord->ExceptionAddress, pException->ExceptionRecord->ExceptionCode, (void*)pException->ContextRecord->Rax,
        (void*)pException->ContextRecord->Rcx, (void*)pException->ContextRecord->Rip);

    return EXCEPTION_EXECUTE_HANDLER;
}

void GlobalTerminateHandler()
{
    SharedUtil::AddDebugLog("Global terminate handler called! Shutting down Atomic AntiCheat...");
}

bool CAtomicAntiCheat::Initialize()
{
    SetUnhandledExceptionFilter(SEHTranslator);
    std::set_terminate(GlobalTerminateHandler);

    m_hProcess = NULL;
    m_iTargetProcessID = NULL;
    //   m_HWIDCache = g_pHWID->LoadHWIDCaches();

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
    time_t tStart = time(NULL);

    m_iTargetProcessID = SharedUtil::GetFivemProcessID();
    while (m_bAlive)
    {
        // Check if the FiveM process is running every 5 seconds without block the pulse thread
        if (time(NULL) - tStart > 4)
        {
            tStart = time(NULL);
            m_iTargetProcessID = SharedUtil::GetFivemProcessID();
        }

        if (m_iTargetProcessID == NULL)
        {
            SharedUtil::AddDebugLog("FiveM closed - stopping scanners!");

            if (m_pGuardManager->IsPulseStarted())
                m_pGuardManager->StopPulse();

            // Clear the detected threats for the next scan session
            m_vDetectedTypes.clear();
            m_pGuardManager->ClearDetections();

            if (m_pAtomicNetwork->GetReadyState() != ix::ReadyState::Closed && m_pAtomicNetwork->GetReadyState() != ix::ReadyState::Closing)
            {
                m_pAtomicNetwork->Disconnect("FiveM Closed");
            }

            if (m_hProcess != NULL && m_hProcess != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_hProcess);
                m_hProcess = NULL;
            }

            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        if (m_hProcess == NULL || m_hProcess == INVALID_HANDLE_VALUE)
        {
            m_hProcess = OpenProcess(PROCESS_VM_READ, FALSE, m_iTargetProcessID);
            if (m_hProcess == NULL || m_hProcess == INVALID_HANDLE_VALUE)
            {
                SharedUtil::AddDebugLog("Failed to attach to fivem process!");

                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
        }

        if (false && !Utils::isFiveMReady())
        {
            SharedUtil::AddDebugLog("FiveM is not ready, waiting...");

            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        if (m_pAtomicNetwork->GetReadyState() == ix::ReadyState::Closed)
        {
            SharedUtil::AddDebugLog("Target Process ID: %d", m_iTargetProcessID);
            if (!m_pAtomicNetwork->Connect())
            {
                SharedUtil::AddDebugLog("Failed to connect to server!");

                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            RunScanners(true);
        }

        if (m_pAtomicNetwork->IsJoinedNetwork() && !m_pGuardManager->IsPulseStarted())
            m_pGuardManager->StartPulse();

        m_pAtomicNetwork->DoPulse();

        if (m_iTargetProcessID == NULL)
        {
            SharedUtil::AddDebugLog("FiveM process exited unexpectedly!");
        }
    }
}

void CAtomicAntiCheat::StartPulse()
{
    // if (m_pGuardManager->IsPulseStarted())
    //     return;

    m_pGuardManager->StartPulse();
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

void CAtomicAntiCheat::Shutdown()
{
    SharedUtil::AddDebugLog("Shutting down Atomic AntiCheat...");

    RunScanners(false);
    if (m_hProcess != NULL && m_hProcess != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_hProcess);
        m_hProcess = NULL;
    }

    m_pGuardManager->StopPulse();
    m_pAtomicNetwork->Disconnect("AntiCheat Shutdown");
    m_bAlive = false;

    // TODO: Unload engine from memory
}

bool CAtomicAntiCheat::IsAtomicThread(HANDLE hThread)
{
    return std::any_of(m_vAtomicThreads.begin(), m_vAtomicThreads.end(), [hThread](CAtomicThread* pThread) { return pThread->GetHandle() == hThread; });
}