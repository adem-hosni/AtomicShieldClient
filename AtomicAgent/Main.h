#pragma once
#include "StdInc.h"

struct SAPIChecksResult
{
    jsoncons::json Status;
    std::string    strTitle;
    std::string    strMessage;
    bool           bSuccess;
    bool           bInitialized;
};

void ApiChecks(LPVOID lpThreadParameter);