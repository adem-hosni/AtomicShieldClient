#pragma once
#include "StdInc.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <mutex>
#include <map>

class CEagleNetwork
{
public:
    CEagleNetwork();
    ~CEagleNetwork();

    bool Connect();
    void SendPacket(eEaglePacketID PacketID, jsoncons::json Data = jsoncons::json());
    jsoncons::json WaitReponse(eEaglePacketID PacketID);

    void OnReceivePacket(const ix::WebSocketMessagePtr& Message);

private:
    ix::WebSocket* m_pWebSocket;
    std::mutex     m_mutex;
    std::condition_variable m_condition;
    std::map<eEaglePacketID, jsoncons::json> m_UnhandledPackets;
};

extern CEagleNetwork* g_pEagleNetwork;