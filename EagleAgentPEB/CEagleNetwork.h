#pragma once
#include "StdInc.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>

class CEagleNetwork
{
public:
    CEagleNetwork();
    ~CEagleNetwork();

    bool Connect();
    void SendPacket(eEaglePacketID PacketID, jsoncons::json Data = jsoncons::json());

    void OnReceivePacket(const ix::WebSocketMessagePtr& Message);

private:
    ix::WebSocket* m_pWebSocket;
};

extern CEagleNetwork* g_pEagleNetwork;