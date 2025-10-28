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

void CAtomicAntiCheat::TerminateSignalHandler(int signal)
{
    g_pAtomicAntiCheat->ForceHardKick(eHardKickReason::PROCESS_TERMINATED);
    g_pAtomicAntiCheat->Shutdown("Process Terminated");
}

BOOL CAtomicAntiCheat::ConsoleHandler(DWORD dwSignal)
{
    if (dwSignal == CTRL_CLOSE_EVENT || dwSignal == CTRL_LOGOFF_EVENT || dwSignal == CTRL_SHUTDOWN_EVENT || dwSignal == CTRL_C_EVENT)
    {
        g_pAtomicAntiCheat->ForceHardKick(eHardKickReason::PROCESS_TERMINATED);
        g_pAtomicAntiCheat->Shutdown();
        return TRUE;
    }
    return FALSE;
}

void CAtomicAntiCheat::SetupExitHandlers()
{
    signal(SIGINT, TerminateSignalHandler);
    signal(SIGTERM, TerminateSignalHandler);
    signal(SIGABRT, TerminateSignalHandler);
    signal(SIGBREAK, TerminateSignalHandler);

    std::atexit([] { TerminateSignalHandler(0); });
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE);
}

bool CAtomicAntiCheat::Initialize()
{
    SharedUtil::AddDebugLog("Initializing Atomic AntiCheat...");
    CCrashHandler::Initialize();
    SetupExitHandlers();

    RtlSecureZeroMemory(&m_ObjAttr, sizeof(m_ObjAttr));
    m_ObjAttr.Length = sizeof(m_ObjAttr);
    RtlSecureZeroMemory(&m_ClientId, sizeof(m_ClientId));

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
    ULONGLONG ullStart = time(NULL);
    ULONGLONG ullGuardHeartbeatCheck = time(NULL);

    m_iTargetProcessID = SharedUtil::GetFivemProcessID();
    while (m_bAlive)
    {
        if (time(NULL) - ullStart > 4)
        {
            ullStart = time(NULL);
            m_iTargetProcessID = SharedUtil::GetFivemProcessID();
        }

        if (m_iTargetProcessID == NULL)
        {
            SharedUtil::AddDebugLog("FiveM closed - stopping scanners!");

            if (m_pGuardManager->IsPulseStarted())
                m_pGuardManager->StopGuards();

            RunScanners(false);

            // Clear the detected threats for the next scan session
            m_vDetectedTypes.clear();
            m_pGuardManager->ClearDetections();

            if (m_pAtomicNetwork->GetReadyState() != ix::ReadyState::Closed && m_pAtomicNetwork->GetReadyState() != ix::ReadyState::Closing)
            {
                m_pAtomicNetwork->Disconnect("FiveM closed");
            }

            SysNtClose(m_hProcess);
            m_hProcess = NULL;

            std::this_thread::sleep_for(std::chrono::seconds(5));
            continue;
        }

        DWORD dwProcessHandleFlags;

        if (!GetHandleInformation(m_hProcess, &dwProcessHandleFlags) || m_hProcess == NULL || m_hProcess == INVALID_HANDLE_VALUE)
        {
            m_ClientId.UniqueProcess = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(m_iTargetProcessID));
            NTSTATUS status = SysNtOpenProcess(&m_hProcess, PROCESS_ALL_ACCESS, &m_ObjAttr, &m_ClientId);
            if (!NT_SUCCESS(status) || !g_pAtomicAntiCheat->IsValidProcessHandle())
            {
                SharedUtil::AddDebugLog("Failed to open FiveM process with ID %d, error: %d", m_iTargetProcessID, status);
                continue;
            }

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
            SharedUtil::AddDebugLog("Connecting to Atomic AntiCheat server...");
            if (!m_pAtomicNetwork->Connect())
            {
                SharedUtil::AddDebugLog("Failed to connect to server!");

                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
            RunScanners(true);
        }

        if (m_pAtomicNetwork->IsJoinedNetwork() && !m_pGuardManager->IsPulseStarted())
        {
            m_pGuardManager->StartGuards();
            SharedUtil::AddDebugLog("Starting Basic Checks...");
            g_pAtomicAntiCheat->StartBasicChecks();
            SharedUtil::AddDebugLog("End Basic Checks");
        }

        if (time(NULL) - ullGuardHeartbeatCheck > 4)
        {
            ullGuardHeartbeatCheck = time(NULL);
            if (m_pGuardManager->GetHeuristicGuard()->IsHeartbeatActive())
            {
                SharedUtil::AddDebugLog("[PING] Heuristic Guard Heartbeat sent!");
                m_pAtomicNetwork->Ping(eHeartbeatType::HEURISTIC_GUARD);
            }
            else
            {
                SharedUtil::AddDebugLog("[FAIL] Process Guard Heartbeat is inactive");
            }

            if (m_pGuardManager->GetProcessGuard()->IsHeartbeatActive())
            {
                SharedUtil::AddDebugLog("[PING] Process Guard Heartbeat sent!");
                m_pAtomicNetwork->Ping(eHeartbeatType::PROCESS_GUARD);
            }
            else
            {
                SharedUtil::AddDebugLog("[FAIL] Process Guard Heartbeat is inactive");
            }
        }

        m_pAtomicNetwork->DoPulse();
        m_pGuardManager->DoPulse();

        if (m_iTargetProcessID == NULL)
        {
            SharedUtil::AddDebugLog("FiveM process exited unexpectedly!");
        }

        if (!m_pAtomicNetwork->IsJoinedNetwork() || m_pAtomicNetwork->GetReadyState() == ix::ReadyState::Closed ||
            m_pAtomicNetwork->GetReadyState() == ix::ReadyState::Closing)
        {
            RunScanners(false);
        }

        if (m_pGuardManager->IsPulseStarted() && m_bRunScanners)
        {
            for (CAtomicThread* thread : m_pGuardManager->GetThreads())
            {
                if (m_bRunScanners && (!thread->IsHandleValid() || thread->IsTerminated() /*|| thread->IsSuspended()*/))
                {
                    SharedUtil::AddDebugLog("Guard thread %d is not valid anymore! Forcing hard kick...", thread->GetThreadID());
                    ForceHardKick(eHardKickReason::GUARD_TIMEDOUT);
                    m_pGuardManager->StopGuards();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    StartPulse();
                    break;
                }
            }
        }
    }
}

void CAtomicAntiCheat::StartPulse()
{
    // if (m_pGuardManager->IsPulseStarted())
    //     return;

    m_pGuardManager->StartGuards();
}

void CAtomicAntiCheat::StartBasicChecks()
{
    BasicChecks::CheckPlugins();

    //  DebugModeEnabled();

    BasicChecks::SecureBootEnabled();

    BasicChecks::TestsigningEnabled();

    BasicChecks::CheckSecurityFeatures();

    CAtomicThread::Create(&BasicChecks::CheckBlacklistedDrivers);
}

void CAtomicAntiCheat::NotifyDetection(eDetectionType DetectionType, std::unordered_map<std::string, ArgType> kwargs, bool bTakeScreenshot)
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

    jsoncons::json RequestData = jsoncons::json::object();
    RequestData["detection_type"] = (int)DetectionType;
    RequestData["report"] = Report;

    if (bTakeScreenshot)
    {
        std::string strScreenshotBuffer;
        char        szError[256];
        memset(szError, 0, sizeof(szError));
        Screenshot::CreateScreenshot(&strScreenshotBuffer, szError);
        RequestData["ss"] = SharedUtil::Base64Encode(strScreenshotBuffer);
        RequestData["error"] = std::string(szError);
    }
    else
    {
        RequestData["ss"] = "";
        RequestData["error"] = "";
    }

    m_pAtomicNetwork->SendPacket(eAtomicPacket::CHEAT_DETECTION, RequestData, true);
}

void CAtomicAntiCheat::ForceHardKick(eHardKickReason KickReason, std::string strOptionalDescription)
{
    jsoncons::json RequestData = jsoncons::json::object();
    RequestData["reason"] = (int)KickReason;
    RequestData["description"] = strOptionalDescription;

    m_pAtomicNetwork->SendPacket(eAtomicPacket::FORCE_HARDKICK, RequestData, true);

    SharedUtil::AddDebugLog("Force Hard Kick issued for reason: %d (%s)", KickReason, strOptionalDescription.c_str());
}

void CAtomicAntiCheat::TerminateThreads()
{
    for (CAtomicThread* pThread : m_vAtomicThreads)
    {
        if (pThread)
        {
            pThread->Terminate();
            delete pThread;
        }
    }
}

void CAtomicAntiCheat::Shutdown(std::string strReason, bool bTerminateThreads, bool bUnload)
{
    SharedUtil::AddDebugLog("Shutting down Atomic AntiCheat: %s", strReason.c_str());

    RunScanners(false);
    if (m_hProcess != NULL && m_hProcess != INVALID_HANDLE_VALUE)
    {
        SysNtClose(m_hProcess);
        m_hProcess = NULL;
    }

    m_pGuardManager->StopGuards();
    m_pAtomicNetwork->Disconnect(strReason);
    m_bAlive = false;

    if (bTerminateThreads)
        TerminateThreads();

    if (bUnload)
        __fastfail(0);
}

bool CAtomicAntiCheat::IsAtomicThread(HANDLE hThread)
{
    return std::any_of(m_vAtomicThreads.begin(), m_vAtomicThreads.end(), [hThread](CAtomicThread* pThread) { return pThread->GetHandle() == hThread; });
}