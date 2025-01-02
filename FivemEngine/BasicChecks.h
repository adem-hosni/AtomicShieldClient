#pragma once
#include "StdInc.h"

namespace BasicChecks
{
    void CheckPlugins();
    void DebugModeEnabled();
    void SecureBootEnabled();
    void TestsigningEnabled();
    void checkBlacklistedDrivers();
};
