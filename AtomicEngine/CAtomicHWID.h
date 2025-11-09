#pragma once

#include "StdInc.h"

class CAtomicHWID
{
public:
    CAtomicHWID();
    ~CAtomicHWID();

    jsoncons::json CollectAllAsJson();

    std::map<std::string, std::string> CollectTPM();
    std::map<std::string, std::string> CollectGPU();
    std::map<std::string, std::string> CollectDisks();
    std::map<std::string, std::string> CollectSMBIOS();
    std::string                        GetCPUSerial();

private:
    std::string                        RunPowerShellCommand(const std::string& cmd);
    std::map<std::string, std::string> QueryWMI(const std::string& wmiClass, const std::vector<std::string>& properties);

    bool TryGetNvidiaGpuUUIDs(std::map<std::string, std::string>& out);

    std::map<std::string, std::string> QueryPhysicalDisks();

    std::map<std::string, std::string> QuerySMBIOSViaWmi();
};