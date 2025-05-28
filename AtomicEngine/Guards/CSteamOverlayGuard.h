#pragma once
#include "StdInc.h"
#include "CGuardBase.h"


class CSteamOverlayGuard final: CGuardBase
{
public:
    CSteamOverlayGuard();
    ~CSteamOverlayGuard();

    static void StaticPulse(void* pContext) { reinterpret_cast<CSteamOverlayGuard*>(pContext)->DoPulse(); }
    void        DoPulse() override;


    void ClearDetections() override {  }
};
