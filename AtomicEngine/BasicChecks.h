#pragma once
#include "StdInc.h"

namespace BasicChecks
{
    void CheckPlugins();
    void DebugModeEnabled();
    void SecureBootEnabled();
    void TestsigningEnabled();
    void CheckBlacklistedDrivers();
    void CheckSecurityFeatures();
};            // namespace BasicChecks
