#include "StdInc.h"

namespace StartupManager
{
    void        StartupFunction();
    bool        IsAppInTaskScheduler();
    bool        AddAppToTaskScheduler();
    bool        RemoveAppFromTaskScheduler();
    std::string GetCurrentProcessName();
}            // namespace StartUpManager
