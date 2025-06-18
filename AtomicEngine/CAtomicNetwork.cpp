#include "CAtomicAntiCheat.h"
#include "CAtomicCore.h"
#include "Common.h"
#include "SharedUtil.h"
#include <condition_variable>
#include <future>

CAtomicNetwork::CAtomicNetwork() : m_bConnected(false), m_bNetworkJoined(false), m_ullLastPingTime(NULL)
{
    m_pWebSocket = new ix::WebSocket();
}

CAtomicNetwork::~CAtomicNetwork()
{
}

bool CAtomicNetwork::Connect()
{
    m_bNetworkJoined = false;
    ix::initNetSystem();

    m_pWebSocket->setUrl(WEBSOCKET_BASE_URL "/c/atomicshieldagent/");

    m_pWebSocket->setOnMessageCallback(std::bind(&CAtomicNetwork::OnReceivePacket, this, std::placeholders::_1));
    m_pWebSocket->setPingInterval(45);
    m_pWebSocket->enablePerMessageDeflate();
    m_pWebSocket->enablePong();
    m_pWebSocket->disableAutomaticReconnection();

    ix::WebSocketInitResult result = m_pWebSocket->connect(32);
    m_pWebSocket->start();

    if (result.success)
    {
        while (!m_bNetworkJoined)
            std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    else
    {
        SharedUtil::AddDebugLog("Couldn't connect to the websocket due to %s", result.errorStr.c_str());
    }

    return result.success;
}

void CAtomicNetwork::SendPacket(eAtomicPacket PacketID, jsoncons::json Data, bool bHighPriority)
{
    // Allocate new json object
    jsoncons::json PacketJson = jsoncons::json::object();

    // Set The Packet type and the unix timestamp
    PacketJson["type"] = (unsigned short)PacketID;
    PacketJson["ut"] = time(NULL);

    // Fill the new json with the data items
    for (const auto& Iter : Data.object_range())
        PacketJson[Iter.key()] = Iter.value();

    // Send the packet to master server
    std::string buffer = g_pAtomicCore->Encrypt(PacketJson.to_string());

    if (bHighPriority)
    {
        m_pWebSocket->send(SharedUtil::Base64Encode(buffer));
        return;
    }
    m_vPendingPackets.push(SharedUtil::Base64Encode(buffer));
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
    while (m_PendingResponses.find(PacketID) == m_PendingResponses.end())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    jsoncons::json Response = m_PendingResponses[PacketID];

    // Check if the unix timestamp received is tampered
    // if (time(NULL) - Response["ut"].as<DWORD>() >= 20)
    //{
    //    __fastfail(0);
    //    // Return an empty data to crash the engine if the __fastfail was tampered
    //    jsoncons::json j;
    //    return j;
    //}

    m_PendingResponses.erase(PacketID);
    return Response;
}

std::string CAtomicNetwork::GetPublicIP()
{
    std::string result;

    HINTERNET hSession = WinHttpOpen(L"AtomicShield/1.0", WINHTTP_ACCESS_TYPE_NO_PROXY, nullptr, nullptr, 0);
    if (!hSession)
    {
        SharedUtil::AddDebugLog("[WinHTTP] WinHttpOpen failed: %lu", GetLastError());
        return "";
    }

    // Optionally enable TLS 1.2 for future HTTPS endpoints
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
    WinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

    // Connect to endpoint
    HINTERNET hConnect = WinHttpConnect(hSession, L"checkip.amazonaws.com", INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!hConnect)
    {
        SharedUtil::AddDebugLog("[WinHTTP] WinHttpConnect failed: %lu", GetLastError());
        WinHttpCloseHandle(hSession);
        return "";
    }

    // Open GET request (HTTP, no SSL flag)
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", nullptr, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest)
    {
        SharedUtil::AddDebugLog("[WinHTTP] WinHttpOpenRequest failed: %lu", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
    {
        SharedUtil::AddDebugLog("[WinHTTP] WinHttpSendRequest failed: %lu", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr))
    {
        SharedUtil::AddDebugLog("[WinHTTP] WinHttpReceiveResponse failed: %lu", GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }

    DWORD dwSize = 0;
    do
    {
        DWORD dwDownloaded = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
        {
            SharedUtil::AddDebugLog("[WinHTTP] WinHttpQueryDataAvailable failed: %lu", GetLastError());
            break;
        }

        if (dwSize == 0)
            break;

        std::string buffer(dwSize, 0);
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
        {
            SharedUtil::AddDebugLog("[WinHTTP] WinHttpReadData failed: %lu", GetLastError());
            break;
        }

        result.append(buffer.c_str(), dwDownloaded);

    } while (dwSize > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (!result.empty())
    {
        // Remove trailing newlines or whitespace
        result.erase(result.find_last_not_of(" \r\n\t") + 1);
    }
    else
    {
        SharedUtil::AddDebugLog("[WinHTTP] No data received from IP API");
    }

    return result;
}

bool CAtomicNetwork::JoinNetwork()
{
    SharedUtil::AddDebugLog("Joining Network...");
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
    RequestData["engine_type"] = 2;                                     // FiveM
    RequestData["build_timestamp"] = CLIENT_BUILD_TIMESTAMP;            // Some players uses an old engine
    RequestData["ip"] = GetPublicIP();

    SendPacket(eAtomicPacket::NETWORK_JOIN, RequestData, true);

    jsoncons::json Response = WaitReponse(NETWORK_JOIN);

    m_bNetworkJoined = Response["success"].as_bool();

    if (m_bNetworkJoined)
    {
        g_pHWID->StoreHWIDCaches(RequestHWID);

        if (!g_pAtomicAntiCheat->GetNetwork()->SyncMaliciousSignatures(Response["signatures"]))
            MessageBox(0, "Failed to sync malicious signatures!", "Error", 0);
    }
    else
    {
        MessageBox(0, Response["message"].as_string().c_str(), "ERROR", MB_ICONERROR);
    }

    SharedUtil::AddDebugLog("Network Join Result -> %d", Response["success"].as_bool());
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
            std::vector<std::string> vSignatures = {};
            for (const auto& element : SignaturesList.array_range())
            {
                vSignatures.push_back(element.as<std::string>());
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

void CAtomicNetwork::HandleEngineShutdown()
{
    g_pAtomicAntiCheat->Shutdown();
}

void CAtomicNetwork::HandleUploadDebugLogs()
{
    jsoncons::json response = jsoncons::json::object();
    std::string    strLogs;

    response["success"] = SharedUtil::GetDebugLogs(strLogs);
    response["logs"] = strLogs;

    SendPacket(REQUEST_DEBUG_LOGS, response);
}

void CAtomicNetwork::HandleRunScanners(jsoncons::json& Packet)
{
}

void CAtomicNetwork::DoPulse()
{
    if (!m_vPendingPackets.empty())
    {
        if (m_pWebSocket->getReadyState() == ix::ReadyState::Open)
        {
            std::string strPacketBuffer = m_vPendingPackets.front();
            m_pWebSocket->send(strPacketBuffer.c_str());
            m_vPendingPackets.pop();
        }
    }

    if (time(NULL) - m_ullLastPingTime > 5)
    {
        m_pWebSocket->ping("Ping");
        m_ullLastPingTime = time(NULL);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

void CAtomicNetwork::OnReceivePacket(const ix::WebSocketMessagePtr& Message)
{
    switch (Message->type)
    {
        case ix::WebSocketMessageType::Open:
        {
            std::thread t(&CAtomicNetwork::OnConnect);
            t.detach();
            break;
        }

        case ix::WebSocketMessageType::Message:
        {
            if (Message->type == ix::WebSocketMessageType::Message)
            {
                std::string    message_buffer = Message->str;
                std::string    decrypted_buffer = g_pAtomicCore->Decrypt(SharedUtil::Base64Decode(message_buffer));
                jsoncons::json json = jsoncons::json::parse(decrypted_buffer);
                HandleIncomingPacket(json);
                m_PendingResponses.insert_or_assign((eAtomicPacket)json["type"].as<int>(), json);
            }
            break;
        }

        case ix::WebSocketMessageType::Close:
        {
            m_bNetworkJoined = false;
            SharedUtil::AddDebugLog("WebSocket Closed: %s (%d)", Message->closeInfo.reason.c_str(), Message->closeInfo.code);
            break;
        }

        case ix::WebSocketMessageType::Error:
            m_pWebSocket->close();
            m_bNetworkJoined = false;
            SharedUtil::AddDebugLog("Error: %s", Message->errorInfo.reason.c_str());
            // Reconnect();
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
        case eAtomicPacket::ENGINE_SHUTDOWN:
            HandleEngineShutdown();
            break;
        case eAtomicPacket::REQUEST_DEBUG_LOGS:
            HandleUploadDebugLogs();
            break;
    }
}

void CAtomicNetwork::Disconnect(std::string strReason)
{
    if (m_pWebSocket->getReadyState() != ix::ReadyState::Closed)
    {
        m_pWebSocket->stop(ix::WebSocketCloseConstants::kNormalClosureCode, strReason);
        m_bNetworkJoined = false;
        m_PendingResponses.clear();
        SharedUtil::AddDebugLog("Disconnect - %s", strReason.c_str());
    }
}
