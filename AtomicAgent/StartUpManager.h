#include "StdInc.h"

namespace StartupManager
{
    void        StartupFunction();
    bool        IsAppInRegistry();
    bool        AddAppToRegistry();
    bool        RemoveAppFromRegistry();
    std::string GetCurrentProcessName();
}            // namespace StartUpManager
