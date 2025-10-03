#pragma once
#include <Windows.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <list>
#include <string>
#include <functional>
#include <mutex>
#include <memory>
#include <windows.h>
#include <tlhelp32.h>
namespace SharedUtil
{
    bool         TerminateProcess(DWORD dwPID);
    int          GetProcessID(const char* szProcessName);
    int          GetFivemProcessID();
    bool         FindStringIC(const std::string& strHaystack, const std::string& strNeedle);
    int          GenerateRandomNumber(int min, int max);
    std::string  GenerateRandomString(int iLength);
    const char*  GetParentProcessName();
    bool         IsRunningAsAdministator();
    void         AddDebugLog(const char* szLog, ...);
    bool         GetDebugLogs(std::string& szLog);
    std::string  GetKnownDirectory(const KNOWNFOLDERID fid);
    bool         SetPrivilege(LPCTSTR lpszPrivilege);
    std::string  Base64Encode(std::string data);
    std::wstring Base64Encode(std::wstring data);
    std::string  Base64Decode(std::string data);
    void         SetRegistryIntValue(const char* ss,const char* szKey, int iValue);
}            // namespace SharedUtil

class Services final
{
public:
    static bool IsDriverRunning(__in const std::wstring& serviceName);            // check if a driver is loaded & in a running state
};

namespace Debugger
{
    enum class DetectionFlags : int
    {
        NONE = 0,
        DEBUG_HARDWARE_REGISTERS = 1,
        DEBUG_DBK64_DRIVER = 2,
        // add more flags here if required
        EXECUTION_ERROR = -1
    };

    class AntiDebug
    {
    public:
        explicit AntiDebug(Settings* s);
        ~AntiDebug();

        AntiDebug(const AntiDebug&) = delete;
        AntiDebug& operator=(const AntiDebug&) = delete;

        Thread*   GetDetectionThread() const;
        Settings* GetSettings() const;

        void StartAntiDebugThread();

        // Detection thread entry and helpers
        static void CheckForDebugger(LPVOID AD);
        static void _IsHardwareDebuggerPresent(LPVOID AD);
        static bool PreventWindowsDebuggers();
        static bool HideThreadFromDebugger(HANDLE hThread);
        static void HideAllThreadsFromDebugger();

        template <typename Func>
        void AddDetectionFunction(Func func)
        {
            std::lock_guard<std::mutex> lock(this->DetectionRoutineMutex);
            DetectionFunctionList.emplace_back(func);
        }

        void RunDetectionFunctions();

        bool IsDBK64DriverLoaded();

    protected:
        std::vector<std::function<DetectionFlags()>> DetectionFunctionList;

        std::list<std::wstring> CommonDebuggerProcesses;

    private:
        std::unique_ptr<Thread>   DetectionThread;
        std::list<DetectionFlags> DetectedMethods;
        Settings*                 Config = nullptr;
        const std::wstring        DBK64Driver = L"DBK64.sys";

        std::mutex DetectionRoutineMutex;
        std::mutex FlggedListMutex;

        void AddFlagged(const DetectionFlags& method)
        {
            std::lock_guard<std::mutex> lock(FlggedListMutex);
            if (std::find(this->DetectedMethods.begin(), this->DetectedMethods.end(), method) == this->DetectedMethods.end())
                this->DetectedMethods.push_back(method);
        }
    };
}            // namespace Debugger


bool Services::IsDriverRunning(__in const std::wstring& serviceName)
{
    if (serviceName.size() == 0)
        return false;

    SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);

    if (!hSCManager)
    {
        Logger::logfw(Warning, L"Failed to open SCM: %d", GetLastError());
        return false;
    }

    //open driver service
    SC_HANDLE hService = OpenService(hSCManager, serviceName.c_str(), SERVICE_QUERY_STATUS);

    if (!hService)
    {
        Logger::logfw(Info, L"Failed to open service %s: %d (this is not an error)", serviceName.c_str(), GetLastError());
        CloseServiceHandle(hSCManager);
        return false;
    }

    SERVICE_STATUS status;

    if (!QueryServiceStatus(hService, &status))     //query the service status
    {
        Logger::logfw(Warning, L"Failed to query service status: %d", GetLastError());
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    return (status.dwCurrentState == SERVICE_RUNNING);
}