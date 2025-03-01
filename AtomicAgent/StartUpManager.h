#include "StdInc.h"

namespace StartupManager
{
    void        StartupFunction();
    bool        IsAppInRegistry(std::string& appName);
    bool        AddAppToRegistry(std::string& appName);
    std::string GetCurrentProcessName();
}            // namespace StartUpManager
