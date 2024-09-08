#include "CEagleNetwork.h"

CEagleNetwork* g_pEagleNetwork = new CEagleNetwork();

CEagleNetwork::CEagleNetwork()
{
    m_pWebSocket = new ix::WebSocket();
}

CEagleNetwork::~CEagleNetwork()
{
}

bool CEagleNetwork::Connect()
{
    if (!ix::initNetSystem())
        return false;
    
    m_pWebSocket->setUrl("ws://127.0.0.1:8000/c/eaglescanner/");

    m_pWebSocket->setOnMessageCallback(std::bind(&CEagleNetwork::OnReceivePacket, this, std::placeholders::_1));

    ix::WebSocketInitResult result = m_pWebSocket->connect(32);

    return result.success;
}

void CEagleNetwork::OnReceivePacket(const ix::WebSocketMessagePtr& Message)
{

}