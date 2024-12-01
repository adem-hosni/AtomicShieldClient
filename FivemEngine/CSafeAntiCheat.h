#pragma once
#include "StdInc.h"
#include "CSafeNetwork.h"
#include "STiming.h"

class CSafeAntiCheat
{
public:
    CSafeAntiCheat();
    ~CSafeAntiCheat();

    CSafeNetwork* GetNetwork() { return m_pSafeNetwork; }
    STiming&       GetTiming() { return m_Timing; }

    bool CheckGameAntiCheatsStatus();

    static void DoPulse();

private:
    STiming m_Timing;

    CSafeNetwork* m_pSafeNetwork;
};

extern CSafeAntiCheat* g_pSafeAntiCheat;