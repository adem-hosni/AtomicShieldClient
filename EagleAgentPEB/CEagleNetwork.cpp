#include "CEagleNetwork.h"
#include <condition_variable>
#include <future>

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
    m_pWebSocket->start();

    return result.success;
}

void CEagleNetwork::SendPacket(eEaglePacketID PacketID, jsoncons::json Data)
{
    // Allocate new json object
    jsoncons::json PacketJson = jsoncons::json::object();

    // Set The Packet Type
    PacketJson["type"] = (unsigned short)PacketID;

    // Fill the new json with the data items
    for (const auto& Iter : Data.object_range())
        PacketJson[Iter.key()] = Iter.value();

    // Send the packet to eagle master server
    m_pWebSocket->send(PacketJson.to_string());
}

jsoncons::json CEagleNetwork::WaitReponse(eEaglePacketID PacketID)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [&] { return m_UnhandledPackets.find(PacketID) != m_UnhandledPackets.end(); });
    jsoncons::json Response = m_UnhandledPackets[PacketID];
    m_UnhandledPackets.erase(PacketID);
    return Response;
}

bool CEagleNetwork::JoinNetwork()
{
    jsoncons::json RequestData = jsoncons::json::object();
    RequestData["mta_serial"] = g_pHWID->GetMTASerial();
    RequestData["username"] = g_pHWID->GetWindowsUsername();
    RequestData["disks"] = g_pHWID->GetDisksSerialNumber();
    RequestData["cpu"] = g_pHWID->GetCPUsSerials();
    RequestData["motherboard_serial"] = g_pHWID->GetMotherBoardSerial();
    RequestData["bios"] = g_pHWID->GetBIOSVersion();

    SendPacket(eEaglePacketID::NETWORK_JOIN, RequestData);
    jsoncons::json Response = WaitReponse(NETWORK_JOIN);

    if (!Response["success"].as_bool())
        MessageBox(0, Response["message"].as_string().c_str(), "ERROR", MB_ICONERROR);

    return Response["success"].as_bool(); 
}

void CEagleNetwork::OnReceivePacket(const ix::WebSocketMessagePtr& Message)
{
    if (Message->type == ix::WebSocketMessageType::Message)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        jsoncons::json              json = jsoncons::json::parse(Message->str);
        m_UnhandledPackets.insert_or_assign((eEaglePacketID)json["type"].as<int>(), json);
        m_condition.notify_one();
    }

    switch (Message->type)
    {
        case ix::WebSocketMessageType::Open:
            printf("Connection eastablished!\n");
            printf("Connection result: %d\n", g_pEagleNetwork->JoinNetwork());
        case ix::WebSocketMessageType::Message:
            std::lock_guard<std::mutex> lock(m_mutex);
            jsoncons::json              json = jsoncons::json::parse(Message->str);
            m_UnhandledPackets.insert_or_assign((eEaglePacketID)json["type"].as<int>(), json);
            m_condition.notify_one();
    }

    
}