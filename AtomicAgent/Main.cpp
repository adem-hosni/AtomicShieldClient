#include "StdInc.h"
#include <string>
#include "Main.h"
#include "SharedChecks.h"
#include "SharedProtocols.h"
#include "StaticAnalysisBypass.h"
#include "CLatencyEvaluator.h"

void ApiChecks(LPVOID lpThreadParameter)
{
    SAPIChecksResult* result = reinterpret_cast<SAPIChecksResult*>(lpThreadParameter);
    result->Status = g_pAtomicAPI->GetStatus();
    result->strTitle = skCrypt("Loading Content Manifest...").decrypt();
    result->strMessage = skCrypt("The agent is loading. This wont take long.").decrypt();
    result->bSuccess = true;

    if (!result->Status["alive"].as_bool())
    {
        result->bSuccess = false;
        if (result->Status.contains("title"))
            result->strTitle = result->Status["title"].as<std::string>();

        if (result->Status.contains("message"))
            result->strMessage = result->Status["message"].as<std::string>();
    }
    else
    {
        if (g_pAtomicAPI->IsValidVersion(PROJECT_VERSION) || true)
        {
            if (g_pAtomicAPI->IsAlreadyConnected())
            {
                result->bSuccess = false;
                result->strTitle = skCrypt("ALREADY CONNECTED");
                result->strMessage = skCrypt("You are already connected to the network.");
            }
        }
        else
        {
            result->bSuccess = false;
            result->strTitle = skCrypt("ATOMICSHIELD UPDATE IN PROGRESS");
            result->strMessage = skCrypt("This version of CashLine is no longer supported. Please update to the latest version to continue.");

            std::string AgentBuffer;
            g_pAtomicAPI->DownloadLatestAgent(&AgentBuffer);
            UpdateManager::InstallAgent(AgentBuffer, result->strTitle, result->strMessage);
        }
    }
    result->bInitialized = true;

    if (result->bSuccess)
        RuntimeImportResolver::ResolveCurrentImports();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    INT CPUInfo[4] = {-1};
    if ((CPUInfo[2] >> 31) & 1 || StaticAnalysisBypass::IsAnalysisVM())
    {
        SharedUtil::AddDebugLog(skCrypt("Analysis VM Detected!"));
        while (true)
        {
            5 + 8;
            8 + 9;
            Sleep(1000);
        }
    }

    CLatencyEvaluator::SetupServerEndPoint([](std::string strBestEndPoint) -> void { g_pAtomicAPI->SetServerEndPoint(strBestEndPoint); }, true);

    std::string processName = StartupManager::GetCurrentProcessName();
    bool        isStartup = false;
    bool        bInitialized = false;
    std::string cmdLine = pCmdLine;
    bool        tos = false;

    SharedUtil::AddDebugLog(skCrypt("CashLine Agent started with command line: %s"), cmdLine.c_str());
    if (cmdLine.find(skCrypt("--old")) != std::string::npos)
    {
        size_t pos = cmdLine.find(skCrypt("--old"));
        if (pos != std::string::npos)
        {
            size_t      endPos = cmdLine.find(' ', pos);
            std::string oldAgentPath = cmdLine.substr(pos + 6, endPos - pos - 6);
            if (!oldAgentPath.empty())
            {
                SharedUtil::AddDebugLog(skCrypt("Deleting old agent at path: %s"), oldAgentPath.c_str());
                DeleteFileA(oldAgentPath.c_str());
            }
        }
    }

    SAPIChecksResult result;
    result.bSuccess = true;
    result.bInitialized = true;

    _beginthread((_beginthread_proc_type)ApiChecks, NULL, &result);

    tos = !CheckIfLoaded("CashLine_TOS");

    if (cmdLine.find(skCrypt("--startup")) != std::string::npos)
    {
        StartupManager::StartupFunction();
    }
    else if (GUI::Initialize())
    {
        GUI::RenderUI(&bInitialized, result.bSuccess, &result.strTitle, &result.strMessage, processName, tos);
    }
    GUI::Destroy();

    return 0;
}