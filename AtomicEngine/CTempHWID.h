#pragma once
#include "StdInc.h"

class CTempHWID
{
public:
    CTempHWID();
    ~CTempHWID();

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
extern CTempHWID* g_pTempHWID;