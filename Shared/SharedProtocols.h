#pragma once
#include <Windows.h>
#include "SharedUtil.h"

namespace SharedProtocols
{
    // Interesting technique which uses the loader & system to block certain types of attacks, such as unsigned modules being injected
    void EnableProcessMitigations();
    void CheckLauncherProcess();
};            // namespace SharedProtocols
