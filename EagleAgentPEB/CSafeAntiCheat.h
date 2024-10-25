#pragma once
#include "StdInc.h"
#include "CSafeNetwork.h"
#include "STiming.h"

class CSafeAntiCheat
{
public:
    CSafeAntiCheat();
    ~CSafeAntiCheat();

    CSafeNetwork* GetEagleNetwork() { return m_pEagleNetwork; }
    STiming&       GetTiming() { return m_Timing; }

    bool CheckGameAntiCheatsStatus();

    static void DoPulse();

private:
    STiming m_Timing;

    CSafeNetwork* m_pEagleNetwork;
};

extern CSafeAntiCheat* g_pEagleAntiCheat;