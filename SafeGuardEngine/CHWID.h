#pragma once
#include "StdInc.h"

class CHWID
{
public:
    CHWID();
    ~CHWID();

    std::string    GetMTASerial();
    jsoncons::json GetExtraData();
    jsoncons::json GetMonitorSerial();
    std::string    GetWindowsUsername();
    std::string    GetMotherBoardSerial();
    jsoncons::json GetDisksSerialNumber();
    std::string    GetCPUsSerials();
    std::string    GetBIOSVersion();
    std::string    GetPNPDeviceID();
    std::string    GetComputerName_();
};
extern CHWID* g_pHWID;