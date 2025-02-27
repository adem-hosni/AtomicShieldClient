#include "CAtomicAntiCheat.h"
#include "CAtomicCore.h"
#include "Common.h"
#include "SharedUtil.h"
#include <condition_variable>
#include <future>

CAtomicNetwork::CAtomicNetwork() : m_bConnected(false), m_bNetworkJoined(false)
{
    m_pWebSocket = new ix::WebSocket();
    CAtomicThread::Create(&CAtomicNetwork::PingPulse, this);
}

CAtomicNetwork::~CAtomicNetwork()
{
}

bool CAtomicNetwork::Connect()
{
    if (!ix::initNetSystem())
        return false;

    m_pWebSocket->setUrl(WEBSOCKET_BASE_URL "/c/atomicshieldagent/");
    
    m_pWebSocket->setOnMessageCallback(std::bind(&CAtomicNetwork::OnReceivePacket, this, std::placeholders::_1));

    m_pWebSocket->setPingInterval(3);
    m_pWebSocket->enablePong();

    ix::WebSocketInitResult result = m_pWebSocket->connect(32);
    m_pWebSocket->start();

    if (result.success)
    {
        while (!m_bNetworkJoined)
            Sleep(25);
    }
    else
    {
        SharedUtil::AddDebugLog("Couldn't connect to the websocket due to %s", result.errorStr.c_str());
    }

    return result.success;
}

void CAtomicNetwork::SendPacket(eAtomicPacket PacketID, jsoncons::json Data)
{
    // Allocate new json object
    jsoncons::json PacketJson = jsoncons::json::object();

    // Set The Packet type and the unix timestamp
    PacketJson["type"] = (unsigned short)PacketID;
    PacketJson["ut"] = time(NULL);

    // Fill the new json with the data items
    for (const auto& Iter : Data.object_range())
        PacketJson[Iter.key()] = Iter.value();

    // Send the packet to eagle master server
    std::string buffer = g_pAtomicCore->Encrypt(PacketJson.to_string());
    m_pWebSocket->send(SharedUtil::Base64Encode(buffer));
}

void CAtomicNetwork::OnConnect()
{
    if (!g_pAtomicAntiCheat->GetNetwork()->JoinNetwork())
    {
        FreeModule(GetModuleHandle(NULL));
        return;
    }   
}

void CAtomicNetwork::StaticPulse(void* pContext)
{
    CAtomicNetwork* pNetwork = reinterpret_cast<CAtomicNetwork*>(pContext);
    pNetwork->DoPulse();
}

jsoncons::json CAtomicNetwork::WaitReponse(eAtomicPacket PacketID)
{
    while (m_UnhandledPackets.find(PacketID) == m_UnhandledPackets.end())
    {
        Sleep(10);
    }
    jsoncons::json Response = m_UnhandledPackets[PacketID];

    // Check if the unix timestamp received is tampered
    if (time(NULL) - Response["ut"].as<DWORD>() >= 20)
    {
        __fastfail(0);
        // Return an empty data to crash the engine if the __fastfail was tampered
        jsoncons::json j;
        return j;
    }

    m_UnhandledPackets.erase(PacketID);
    return Response;
}

bool CAtomicNetwork::JoinNetwork()
{
    jsoncons::json RequestHWID;
    RequestHWID["extra"] = g_pHWID->GetExtraData();
    RequestHWID["username"] = g_pHWID->GetWindowsUsername();
    RequestHWID["disks"] = g_pHWID->GetDisksSerialNumber();
    RequestHWID["cpu"] = g_pHWID->GetCPUsSerials();
    RequestHWID["motherboard_serial"] = g_pHWID->GetMotherBoardSerial();
    RequestHWID["bios"] = g_pHWID->GetBIOSVersion();
    RequestHWID["pnp_device"] = g_pHWID->GetPNPDeviceID();
    RequestHWID["computer_name"] = g_pHWID->GetComputerName_();
    RequestHWID["monitor"] = g_pHWID->GetMonitorSerial();

    jsoncons::json RequestData;
    RequestData["hwid"] = RequestHWID;
    RequestData["cache"] = g_pAtomicAntiCheat->GetCurrentHWIDCache();
    RequestData["engine_type"] = 2;            // FiveM

    SendPacket(eAtomicPacket::NETWORK_JOIN, RequestData);

    jsoncons::json Response = WaitReponse(NETWORK_JOIN);

    m_bNetworkJoined = Response["success"].as_bool();

    if (m_bNetworkJoined)
    {
        g_pHWID->StoreHWIDCaches(RequestHWID);
        g_pAtomicAntiCheat->StartPulse();
        
        if (!g_pAtomicAntiCheat->GetNetwork()->SyncMaliciousSignatures(Response["signatures"]))
            MessageBox(0, "Failed to sync malicious signatures!", "Error", 0);
    }
    else
    {
        MessageBox(0, Response["message"].as_string().c_str(), "ERROR", MB_ICONERROR);
    }

    return m_bNetworkJoined;
}

bool CAtomicNetwork::SyncMaliciousSignatures(jsoncons::json& Signatures)
{
    for (const auto& Item : Signatures.object_range())
    {
        const std::string&    SignatureTitle = Item.key();
        const jsoncons::json& SignaturesList = Item.value();

        if (SignaturesList.is_array())
        {
            std::vector<std::wstring> vSignatures = {};
            for (const auto& element : SignaturesList.array_range())
            {
                vSignatures.push_back(element.as<std::wstring>());
            }
            m_Signatures[SignatureTitle] = vSignatures;
            vSignatures.clear();
        }
    }
    g_pAtomicAntiCheat->GetGuardManager()->GetHeuristicGuard()->AddSignatures(m_Signatures);

    Signatures.clear();
    m_Signatures.clear();

    return true;
}

void CAtomicNetwork::HandleRequestScreenshot()
{
    jsoncons::json response = jsoncons::json::object();
    char           szError[256];
    memset(szError, 0, sizeof(szError));

    std::string strScreenshotBuffer;
    bool        bSuccess = Screenshot::CreateScreenshot(&strScreenshotBuffer, szError);

    response["success"] = bSuccess;
    response["message"] = (const char*)szError;
    response["buffer"] = SharedUtil::Base64Encode(strScreenshotBuffer);

    SendPacket(REQUEST_SCREENSHOT, response);
}

void CAtomicNetwork::HandleRunScanners(jsoncons::json& Packet)
{
    jsoncons::json response = jsoncons::json::object();
    response["success"] = false;

    if (Packet.contains(skCrypt("run").decrypt()))
    {
        SharedUtil::AddDebugLog("Running Scanenrs");
        g_pAtomicAntiCheat->RunScanners(Packet["run"].as_bool());
        response["success"] = true;
    }

    SendPacket(RUN_SCANNERS, response);
}

void CAtomicNetwork::DoPulse()
{
    // printf("state: %d\n", m_pWebSocket->getReadyState());
}

void CAtomicNetwork::PingPulse(LPVOID lpContext)
{
    CAtomicNetwork* pAtomicNetwork = reinterpret_cast<CAtomicNetwork*>(lpContext);
    while (true)
    {
        if (pAtomicNetwork->GetWebSocket()->getReadyState() == ix::ReadyState::Open)
        {
            pAtomicNetwork->GetWebSocket()->ping("ping");
            pAtomicNetwork->GetWebSocket()->ping("");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    }
}

void CAtomicNetwork::OnReceivePacket(const ix::WebSocketMessagePtr& Message)
{
    switch (Message->type)
    {
        case ix::WebSocketMessageType::Open:
        {
            _beginthread((_beginthread_proc_type)&CAtomicNetwork::OnConnect, NULL, this);
            break;
        }

        case ix::WebSocketMessageType::Message:
        {
            if (Message->type == ix::WebSocketMessageType::Message)
            {
                std::string                 message_buffer = Message->str;
                std::string                 decoded_buffer = SharedUtil::Base64Decode(message_buffer);
                std::string                 decrypted_buffer = g_pAtomicCore->Decrypt(decoded_buffer);
                jsoncons::json              json = jsoncons::json::parse(decrypted_buffer);
                HandleIncomingPacket(json);
                m_UnhandledPackets.insert_or_assign((eAtomicPacket)json["type"].as<int>(), json);
            }
            break;
        }

        case ix::WebSocketMessageType::Close:
        {
            m_pWebSocket->close();
            SharedUtil::AddDebugLog("WebSocket Closed: %s (%d)", Message->closeInfo.reason.c_str(), Message->closeInfo.code);
            Reconnect();
            break;
        }

        case ix::WebSocketMessageType::Error:
            m_pWebSocket->close();
            SharedUtil::AddDebugLog("Error: %s", Message->errorInfo.reason.c_str());
            Reconnect();
            break;
    }
}

void CAtomicNetwork::HandleIncomingPacket(jsoncons::json Packet)
{
    if (!Packet.contains("type"))
        return;

    int iPacketID = Packet["type"].as<int>();
    switch ((eAtomicPacket)iPacketID)
    {
        case eAtomicPacket::REQUEST_SCREENSHOT:
            HandleRequestScreenshot();
            break;
        case eAtomicPacket::RUN_SCANNERS:
            HandleRunScanners(Packet);
            break;
    }
}

void CAtomicNetwork::Reconnect()
{
    m_bNetworkJoined = false;

    SharedUtil::AddDebugLog("Attempting to reconnect to the websocket...");
    while (m_pWebSocket->getReadyState() == ix::ReadyState::Closed || m_pWebSocket->getReadyState() == ix::ReadyState::Closing)
    {
        Connect();
        Sleep(2 * 1000);
    }
    SharedUtil::AddDebugLog("Websocket connection established successfuly!");
}