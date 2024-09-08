#pragma once
#include "StdInc.h"

class CHWID
{
public:
    CHWID();
    ~CHWID();

    std::string        GetMTASerial();
    std::string        GetWindowsUsername();
    std::string        GetMotherBoardSerial();
    jsoncons::json GetDisksSerialNumber();
    std::string        GetCPUsSerials();
    std::string        GetBIOSVersion();
};

extern CHWID* g_pHWID;