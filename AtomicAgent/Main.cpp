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
    SharedProtocols::EnableProcessMitigations();

    // Check the launcher process (for anti-debugging)
    SharedProtocols::CheckLauncherProcess();

    jsoncons::json Status = g_pAtomicAPI->GetStatus();

    std::string strTitle = "ERROR";
    std::string strMessage = "Unknown Error!";
    bool        bError = true;

    if (!Status["alive"].as_bool())
    {
        bError = false;
        if (Status.contains("title"))
            strTitle = Status["title"].as<std::string>();

        if (Status.contains("message"))
            strMessage = Status["message"].as<std::string>();
    }
    else
    {
        if (g_pAtomicAPI->IsAlreadyConnected())
        {
            bError = false;
            strTitle = "ALREADY CONNECTED";
            strMessage = "You are already connected to the AtomicShield network.";
        }
    }

    if (GUI::Initialize())
    {
        GUI::RenderUI(!bError, strTitle, strMessage);
    }
    GUI::Destroy();

    return 0;
}