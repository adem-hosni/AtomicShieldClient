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
    std::string    GetSteamID();

    void           StoreHWIDCaches(jsoncons::json hwid);
    jsoncons::json LoadHWIDCaches();

private:
    bool        WriteADS(std::string strPath, std::string strStreamName, std::string strData);
    std::string ReadADS(std::string strPath, std::string strStreamName);
};
extern CHWID* g_pHWID;