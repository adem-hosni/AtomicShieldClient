#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <list>
#include <functional>
#include <thread>
#include <mutex>
#include <chrono>
#include <winternl.h>

enum DetectionFlags
{
    NONE = 0,
    DEBUG_HARDWARE_REGISTERS = 1,
    DEBUG_DBK64_DRIVER = 2,
    EXECUTION_ERROR = 100
};

class AntiDebug
{
public:
    AntiDebug()
    {
        if (!PreventWindowsDebuggers())
        {
            // Failed to patch debug routines
        }

        CommonDebuggerProcesses.push_back(L"x64dbg.exe");
        CommonDebuggerProcesses.push_back(L"CheatEngine.exe");
        CommonDebuggerProcesses.push_back(L"idaq64.exe");
        CommonDebuggerProcesses.push_back(L"cheatengine-x86_64-SSE4-AVX2.exe");
        CommonDebuggerProcesses.push_back(L"kd.exe");
        CommonDebuggerProcesses.push_back(L"DbgX.Shell.exe");
    }

    ~AntiDebug()
    {
        if (Monitoring)
        {
            Monitoring = false;
            if (DetectionThread.joinable())
            {
                DetectionThread.join();
            }
        }
    }

    void StartAntiDebugMonitoring()
    {
        Monitoring = true;
        DetectionThread = std::thread(&AntiDebug::CheckForDebugger, this);
    }

    void StopAntiDebugMonitoring()
    {
        Monitoring = false;
        if (DetectionThread.joinable())
        {
            DetectionThread.join();
        }
    }

    static bool PreventWindowsDebuggers();
    static bool HideThreadFromDebugger(HANDLE hThread);
    static void HideAllThreadsFromDebugger();
    bool        IsDBK64DriverLoaded();

    template <typename Func>
    void AddDetectionFunction(Func func)
    {
        std::lock_guard<std::mutex> lock(DetectionRoutineMutex);
        DetectionFunctionList.emplace_back(func);
    }

    void RunDetectionFunctions()
    {
        std::lock_guard<std::mutex> lock(DetectionRoutineMutex);

        for (const auto& func : this->DetectionFunctionList)
        {
            DetectionFlags DetectedMethod = NONE;

            if (DetectedMethod = func())
            {
                if (DetectedMethod > EXECUTION_ERROR)
                {
                    // Debugger detected - take action here
                    OnDebuggerDetected(DetectedMethod);
                }
            }
        }
    }

    static void CheckHardwareDebugRegisters();

protected:
    virtual void OnDebuggerDetected(DetectionFlags method)
    {
        // Override this in derived class to handle detections
        // Default behavior can be termination, obfuscation, etc.
    }

private:
    void CheckForDebugger();

    std::vector<std::function<DetectionFlags()>> DetectionFunctionList;
    std::list<std::wstring>                      CommonDebuggerProcesses;

    std::thread DetectionThread;
    std::mutex  DetectionRoutineMutex;
    bool        Monitoring = false;

    const std::wstring DBK64Driver = L"DBK64.sys";
};

extern AntiDebug g_AntiDebug;      
