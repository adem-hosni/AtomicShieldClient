#pragma once
#include <Windows.h>
#include <queue>

class CAtomicLogger
{
public:
    enum eLogType
    {
        LOG_INFO,
        LOG_DEBUG,
        LOG_ERROR,
        LOG_WARNING
    };

    struct SLogEntry
    {
        eLogType LogType;
        char*     szLog;
    };

    struct SEncodedLogEntry
    {
        FILE* hFileHandle;
        char* szEncodedLog;
    };

    CAtomicLogger();
    ~CAtomicLogger();

    void Initialize();
    void InitializeFiles();
    void CollectHandles();

    void EncoderPulse();
    void DoPulse();

    void        GetLogInfo(eLogType LogType, char* szLogLevel, FILE* hFileHandle);

    void PreLog(eLogType LogType, const char* szLog, ...);
    void Log(eLogType LogType, const char* szLog, ...);
    void AddInfoLog(const char* szLog, ...);

private:
    std::queue<SLogEntry> m_LogQueue;
    std::queue<SEncodedLogEntry> m_EncodedLogQuee;

    char szDebugLogDirectory[MAX_PATH];
    char szInfoLogPath[MAX_PATH];
    char szDebugLogPath[MAX_PATH];
    char szErrorLogPath[MAX_PATH];
    char szWarningLogPath[MAX_PATH];

    FILE* hInfoLogFile;
    FILE* hDebugLogFile;
    FILE* hErrorLogFile;
    FILE* hWarningLogFile;
};

extern CAtomicLogger* g_pAtomicLogger;