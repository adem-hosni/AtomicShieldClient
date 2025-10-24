#pragma once
#include "CAtomicLogger.h"

namespace SharedLogger
{
    void LogDebug(const char* fmt, ...);
    void LogInfo(const char* fmt, ...);
    void LogError(const char* fmt, ...);
    void LogWarning(const char* fmt, ...);
};
