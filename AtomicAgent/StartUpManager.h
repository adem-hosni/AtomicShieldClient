#include "StdInc.h"

namespace StartUpManager
{
    void        StartupFunction(bool bNoErrors, std::string strErrorTitle, std::string strErrorDescription);
    bool        IsAppInRegistry(std::string& appName);
    bool        AddAppToRegistry(std::string& appName);
    std::string GetCurrentProcessName();
}            // namespace StartUpManager
