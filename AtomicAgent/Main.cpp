#include "StdInc.h"
#include "Main.h"
#include "SharedChecks.h"
#include "SharedProtocols.h"

int main(int argc, char* argv[])
{
#ifdef _DEBUG
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif

    // Enable microsoft process mitigations (Avoid unsigned code execution, ...)
    // SharedProtocols::EnableProcessMitigations();

    // Check the launcher process (for anti-debugging)
    SharedProtocols::CheckLauncherProcess();

    jsoncons::json Status = g_pAtomicAPI->GetStatus();
    std::string   processName = StartUpManager::GetCurrentProcessName();
    std::string    strTitle = "ERROR";
    std::string    strMessage = "Unknown Error!";
    bool           bSuccess = true;
    bool           isStartup = false;

    if (!Status["alive"].as_bool())
    {
        bSuccess = false;
        if (Status.contains("title"))
            strTitle = Status["title"].as<std::string>();

        if (Status.contains("message"))
            strMessage = Status["message"].as<std::string>();
    }
    else
    {
        if (g_pAtomicAPI->IsValidVersion(PROJECT_VERSION))
        {
            if (g_pAtomicAPI->IsAlreadyConnected())
            {
                bSuccess = false;
                strTitle = "ALREADY CONNECTED";
                strMessage = "You are already connected to the network.";
            }
        }
        else
        {
            bSuccess = false;
            strTitle = "OUTDATED VERSION";
            strMessage = "This version of AtomicShield is no longer supported. Please update to the latest version to continue.";
        }
    }
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--startup") == 0)
        {
            isStartup = true;
            break;
        }
    }

    if (isStartup)
    {
        StartUpManager::StartupFunction(!bSuccess, strTitle, strMessage);
        return 0;
    }
    if (GUI::Initialize())
    {
        GUI::RenderUI(!bSuccess, strTitle, strMessage, processName);
    }
    GUI::Destroy();

    return 0;
}