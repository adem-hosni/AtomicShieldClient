#include "CEagleAntiCheat.h"

CEagleAntiCheat* g_pEagleAntiCheat = new CEagleAntiCheat();

CEagleAntiCheat::CEagleAntiCheat()
{
    m_pEagleNetwork = new CEagleNetwork();
}

CEagleAntiCheat::~CEagleAntiCheat()
{
    if (m_pEagleNetwork)
        delete m_pEagleNetwork;
}
