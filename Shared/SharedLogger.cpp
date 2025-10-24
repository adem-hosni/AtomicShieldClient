#include "SharedLogger.h"
#include "SharedUtil.h"

void SharedLogger::LogDebug(const char* fmt, ...)
{
    SharedUtil::AddDebugLog("LogDebug called");
    va_list args;
    va_start(args, fmt);
    SharedUtil::AddDebugLog("Before g_pAtomicLogger Log call");
    g_pAtomicLogger->Log(CAtomicLogger::LOG_DEBUG, fmt, args);
    va_end(args);
}

void SharedLogger::LogInfo(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    g_pAtomicLogger->Log(CAtomicLogger::LOG_INFO, fmt, args);
    va_end(args);
}

void SharedLogger::LogError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    g_pAtomicLogger->Log(CAtomicLogger::LOG_ERROR, fmt, args);
    va_end(args);
}

void SharedLogger::LogWarning(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    g_pAtomicLogger->Log(CAtomicLogger::LOG_WARNING, fmt, args);
    va_end(args);
}
