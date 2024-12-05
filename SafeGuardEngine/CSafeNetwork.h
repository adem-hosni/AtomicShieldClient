#pragma once
#include "StdInc.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>
#include <mutex>
#include <map>

class CSafeNetwork
{
public:
    CSafeNetwork();
    ~CSafeNetwork();

    bool           Connect();
    bool           IsConnected() { return m_bConnected; }
    void           SendPacket(eSafePacketID PacketID, jsoncons::json Data = jsoncons::json());
    jsoncons::json WaitReponse(eSafePacketID PacketID);

    static void OnConnect();
    void DoPulse();

    bool JoinNetwork();
    bool SyncMaliciousSignatures();

    void OnReceivePacket(const ix::WebSocketMessagePtr& Message);

    std::map<std::string, std::vector<std::string>> GetSignatures() { return m_Signatures; }

private:
    ix::WebSocket*                                  m_pWebSocket;
    std::mutex                                      m_mutex;
    std::condition_variable                         m_condition;
    std::map<eSafePacketID, jsoncons::json>        m_UnhandledPackets;
    std::map<std::string, std::vector<std::string>> m_Signatures;

    bool m_bConnected;
};

extern CSafeNetwork* g_pSafeNetwork;