#pragma once
#include "StdInc.h"
#include "CEagleNetwork.h"
#include "STiming.h"

class CEagleAntiCheat
{
public:
    CEagleAntiCheat();
    ~CEagleAntiCheat();

    CEagleNetwork* GetEagleNetwork() { return m_pEagleNetwork; }
    STiming&       GetTiming() { return m_Timing; }

    bool CheckGameAntiCheatsStatus();

    static void DoPulse();

private:
    STiming m_Timing;

    CEagleNetwork* m_pEagleNetwork;
};

extern CEagleAntiCheat* g_pEagleAntiCheat;