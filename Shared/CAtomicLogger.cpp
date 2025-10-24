#include "CAtomicLogger.h"
#include "SharedUtil.h"
#include "CAtomicCore.h"
#include <process.h>
#include <ctime>
#include <string>
#include <ShlObj.h>

CAtomicLogger* g_pAtomicLogger = new CAtomicLogger();

CAtomicLogger::CAtomicLogger()
{
    hInfoLogFile = nullptr;
    hDebugLogFile = nullptr;
    hErrorLogFile = nullptr;
    hWarningLogFile = nullptr;

    memset(szInfoLogPath, 0, sizeof(szInfoLogPath));
    memset(szDebugLogPath, 0, sizeof(szDebugLogPath));
    memset(szErrorLogPath, 0, sizeof(szErrorLogPath));
    memset(szWarningLogPath, 0, sizeof(szWarningLogPath));
}

CAtomicLogger::~CAtomicLogger()
{
    if (hInfoLogFile)
        fclose(hInfoLogFile);
    if (hDebugLogFile)
        fclose(hDebugLogFile);
    if (hErrorLogFile)
        fclose(hErrorLogFile);
    if (hWarningLogFile)
        fclose(hWarningLogFile);
}

void CAtomicLogger::Initialize()
{
    char szLocalAppDataPath[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, szLocalAppDataPath)))
    {
        strcpy(szLocalAppDataPath, ".");
    }

    sprintf(szDebugLogDirectory, "%s\\AtomicShield\\", szLocalAppDataPath);
    CreateDirectory(szDebugLogDirectory, NULL);

    InitializeFiles();
    CollectHandles();

    _beginthread(
        [](void* pArg) -> void
        {
            CAtomicLogger* pLogger = (CAtomicLogger*)pArg;
            pLogger->EncoderPulse();
        },
        0, this);
    _beginthread(
        [](void* pArg) -> void
        {
            CAtomicLogger* pLogger = (CAtomicLogger*)pArg;
            pLogger->DoPulse();
        },
        0, this);
}

void CAtomicLogger::InitializeFiles()
{
    time_t t = std::time(nullptr);
    tm     now{};

    if (localtime_s(&now, &t) != 0)
    {
        SharedUtil::AddDebugLog("localtime_s failed, using fallback timestamp.");
        memset(&now, 0, sizeof(now));
    }

    // Safe formatted filenames
    sprintf(szInfoLogPath, "%sInfoLog_%04d_%02d_%02d_%02d-%02d.log", szDebugLogDirectory, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour,
            now.tm_min);

    sprintf(szDebugLogPath, "%sDebugLog_%04d_%02d_%02d_%02d-%02d.log", szDebugLogDirectory, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour,
            now.tm_min);

    sprintf(szErrorLogPath, "%sErrorLog_%04d_%02d_%02d_%02d-%02d.log", szDebugLogDirectory, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour,
            now.tm_min);

    sprintf(szWarningLogPath, "%sWarningLog_%04d_%02d_%02d_%02d-%02d.log", szDebugLogDirectory, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, now.tm_hour,
            now.tm_min);

    // Delete old log files if they exist
    for (const auto& path : {szInfoLogPath, szDebugLogPath, szErrorLogPath, szWarningLogPath})
    {
        DeleteFileA(path);
    }
}

void CAtomicLogger::CollectHandles()
{
    if (hInfoLogFile)
    {
        fclose(hInfoLogFile);
        hInfoLogFile = nullptr;
    }
    if (hDebugLogFile)
    {
        fclose(hDebugLogFile);
        hDebugLogFile = nullptr;
    }
    if (hErrorLogFile)
    {
        fclose(hErrorLogFile);
        hErrorLogFile = nullptr;
    }
    if (hWarningLogFile)
    {
        fclose(hWarningLogFile);
        hWarningLogFile = nullptr;
    }

    hInfoLogFile = fopen(szInfoLogPath, "a+");
    hDebugLogFile = fopen(szDebugLogPath, "a+");
    hErrorLogFile = fopen(szErrorLogPath, "a+");
    hWarningLogFile = fopen(szWarningLogPath, "a+");
}

void CAtomicLogger::PreLog(eLogType LogType, const char* szLog, ...)
{
    char    szFormatted[1536];
    va_list Args;
    va_start(Args, szLog);
    vsnprintf(szFormatted, sizeof(szFormatted), szLog, Args);
    va_end(Args);

    m_LogQueue.push({LogType, szFormatted});
}

void CAtomicLogger::GetLogInfo(eLogType LogType, char* szLogLevel, FILE* hFileHandle)
{
    switch (LogType)
    {
        case LOG_INFO:
            hFileHandle = hInfoLogFile;
            szLogLevel = (char*)"[INFO]";
            break;
        case LOG_DEBUG:
            hFileHandle = hDebugLogFile;
            szLogLevel = (char*)"[DEBUG]";
            break;
        case LOG_ERROR:
            hFileHandle = hErrorLogFile;
            szLogLevel = (char*)"[ERROR]";
            break;
        case LOG_WARNING:
            hFileHandle = hWarningLogFile;
            szLogLevel = (char*)"[WARNING]";
            break;
    }
}

void CAtomicLogger::Log(eLogType LogType, const char* szLog, ...)
{
    char  szLogLevel[128];
    FILE* hFile = nullptr;
    SharedUtil::AddDebugLog("CAtomicLogger::Log called");
    GetLogInfo(LogType, szLogLevel, hFile);

    if (hFile)
    {
        time_t t = std::time(0);
        tm*    now = std::localtime(&t);
        char   szTimestamp[600];
        memset(szTimestamp, 0, sizeof(szTimestamp));
        sprintf(szTimestamp, "[%04d-%02d-%02d %02d:%02d:%02d] %s %s\n", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min,
                now->tm_sec, szLogLevel, szLog);
        va_list args;
        va_start(args, szLog);
        vfprintf(hFile, szTimestamp, args);
        fflush(hFile);
        va_end(args);
    }
}

void CAtomicLogger::EncoderPulse()
{
    char  szLogBuffer[1536];
    char  szLogLevel[128];
    FILE* hFile = nullptr;

    while (true)
    {
        while (!m_LogQueue.empty())
        {
            hFile = nullptr;
            memset(szLogBuffer, 0, sizeof(szLogBuffer));
            memset(szLogLevel, 0, sizeof(szLogLevel));

            time_t t = std::time(0);
            tm*    now = std::localtime(&t);

            SLogEntry& LogEntry = m_LogQueue.front();
            GetLogInfo(LogEntry.LogType, szLogLevel, hFile);

            sprintf(szLogBuffer, "[%04d-%02d-%02d %02d:%02d:%02d] %s %s\n", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min,
                    now->tm_sec, szLogLevel, LogEntry.szLog);

            m_EncodedLogQuee.push({hFile, g_pAtomicCore->Encrypt(szLogBuffer).data()});

            m_LogQueue.pop();
        }
    }
}

void CAtomicLogger::DoPulse()
{
    while (true)
    {
        while (!m_EncodedLogQuee.empty())
        {
            SEncodedLogEntry& EncodedLogEntry = m_EncodedLogQuee.front();
            if (EncodedLogEntry.hFileHandle)
            {
                fprintf(EncodedLogEntry.hFileHandle, EncodedLogEntry.szEncodedLog);
                fclose(EncodedLogEntry.hFileHandle);
            }
            m_EncodedLogQuee.pop();
        }
    }
}