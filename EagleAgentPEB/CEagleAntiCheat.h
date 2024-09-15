#pragma once
#include "StdInc.h"
#include "CEagleNetwork.h"

class CEagleAntiCheat
{
public:
    CEagleAntiCheat();
    ~CEagleAntiCheat();

    CEagleNetwork* GetEagleNetwork() { return m_pEagleNetwork; }

    bool CheckGameAntiCheatsStatus();

private:
    CEagleNetwork* m_pEagleNetwork;
};

extern CEagleAntiCheat* g_pEagleAntiCheat;