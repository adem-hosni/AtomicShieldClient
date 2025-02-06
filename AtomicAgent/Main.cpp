#include "StdInc.h"
#include "Main.h"
#include "SharedChecks.h"
#include "SharedProtocols.h"

//int main(int argc, char* argv[])
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
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
    std::string    processName = StartupManager::GetCurrentProcessName();
    std::string    strTitle = skCrypt("ERROR").decrypt();
    std::string    strMessage = skCrypt("Unknown Error!").decrypt();
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
                strTitle = skCrypt("ALREADY CONNECTED");
                strMessage = skCrypt("You are already connected to the network.");
            }
        }
        else
        {
            bSuccess = false;
            strTitle = skCrypt("OUTDATED VERSION");
            strMessage = skCrypt("This version of AtomicShield is no longer supported. Please update to the latest version to continue.");
        }
    }
    
    
    if (GUI::Initialize())
    {
        GUI::RenderUI(!bSuccess, strTitle, strMessage, processName);
    }
    GUI::Destroy();

    return 0;
}