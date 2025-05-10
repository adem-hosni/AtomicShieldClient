#include "StdInc.h"
#include "Main.h"
#include "SharedChecks.h"
#include "SharedProtocols.h"
#include "StaticAnalysisBypass.h"
#include <vmaware.hpp>

void ApiChecks(LPVOID lpThreadParameter)
{
    SAPIChecksResult* result = reinterpret_cast<SAPIChecksResult*>(lpThreadParameter);
    result->Status = g_pAtomicAPI->GetStatus();
    result->strTitle = skCrypt("ERROR").decrypt();
    result->strMessage = skCrypt("Unknown Error!").decrypt();
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
        if (g_pAtomicAPI->IsValidVersion(PROJECT_VERSION))
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
            result->strTitle = skCrypt("OUTDATED VERSION");
            result->strMessage = skCrypt("This version of AtomicShield is no longer supported. Please update to the latest version to continue.");
        }
    }
    result->bInitialized = true;
}

// int main(int argc, char* argv[])
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
#ifdef _DEBUG
#endif
     AllocConsole();
     freopen("CONIN$", "r", stdin);
     freopen("CONOUT$", "w", stdout);
     freopen("CONOUT$", "w", stderr);

    // Enable microsoft process mitigations (Avoid unsigned code execution, ...)
    // SharedProtocols::EnableProcessMitigations();

    // Check the launcher process (for anti-debugging)
    //  SharedProtocols::CheckLauncherProcess();

        INT CPUInfo[4] = {-1};

    //__cpuid(CPUInfo, 1);

    if ((CPUInfo[2] >> 31) & 1 || StaticAnalysisBypass::IsAnalysisVM())
    {
        while (true)
        {
            // Perform some math operations to keep the CPU busy like a legit process hahahaha
            5 + 8;
            8 + 9;
            Sleep(1000);
        }
    }


        if (VM::detect())
        {
            while (true)
            {
                5 + 8;
                8 + 9;
                Sleep(1000);
            }
        }


    RuntimeImportResolver::ResolveCurrentImports();

    std::string processName = StartupManager::GetCurrentProcessName();
    bool        isStartup = false;
    bool        bInitialized = false;
    std::string cmdLine = pCmdLine;

    SAPIChecksResult result;
    result.bSuccess = true;
    result.bInitialized = true;
    _beginthread((_beginthread_proc_type)ApiChecks, NULL, &result);

    if (cmdLine.find(skCrypt("--startup")) != std::string::npos)
    {
        StartupManager::StartupFunction();
    }
    else if (GUI::Initialize())
    {
        GUI::RenderUI(&bInitialized, result.bSuccess, result.strTitle, result.strMessage, processName);
    }
    GUI::Destroy();

    return 0;
}