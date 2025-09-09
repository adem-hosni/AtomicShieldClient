#include "CAtomicAPI.h"
#include "CAtomicCore.h"
#include "skCrypter.h"
#include "Common.h"
#include "SharedUtil.h"

CAtomicAPI* g_pAtomicAPI = new CAtomicAPI();

CAtomicAPI::CAtomicAPI()
{
    m_strServerEndPoint = API_BASE_URL;
}

CAtomicAPI::~CAtomicAPI()
{
}

jsoncons::json CAtomicAPI::GetStatus()
{
    auto        sk_title = skCrypt("Connection Error");
    auto        sk_message = skCrypt("Failed to connect to CashLine Server");
    std::string end = m_strServerEndPoint + std::string(skCrypt("/anticheat/status/agent").decrypt());
    std::string buffer = PostRequest(end.c_str(), jsoncons::json());
    if (buffer.empty())
    {
        jsoncons::json JsonResponse = jsoncons::json::object();
        JsonResponse["alive"] = false;
        JsonResponse["title"] = std::string(sk_title.decrypt());
        JsonResponse["message"] = std::string(sk_message.decrypt());

        return JsonResponse;
    }
    return jsoncons::json::parse(buffer);
}

bool CAtomicAPI::IsAlreadyConnected()
{
    std::string buffer = PostRequest(m_strServerEndPoint + "/anticheat/status/isconnected");
    if (buffer.empty())
    {
        return false;
    }
    jsoncons::json JsonResponse = jsoncons::json::parse(buffer);

    return JsonResponse["success"].as<bool>();
}

bool CAtomicAPI::IsValidVersion(const char* szVersion)
{
    jsoncons::json RequestBody = jsoncons::json::object();
    RequestBody["version"] = szVersion;
    std::string buffer = PostRequest(m_strServerEndPoint + "/anticheat/status/version", RequestBody);

    if (buffer.empty())
    {
        return false;
    }
    jsoncons::json JsonResponse = jsoncons::json::parse(buffer);
    return JsonResponse["success"].as<bool>();
}

void CAtomicAPI::DownloadEngine(std::string* buffer, SUserData* pUserData)
{
    *buffer = PostRequest(m_strServerEndPoint + "/resources/scan/fivem", jsoncons::json(), pUserData);
}

void CAtomicAPI::DownloadLatestAgent(std::string* buffer)
{
    *buffer = PostRequest(m_strServerEndPoint + "/resources/latest-agent", jsoncons::json(), nullptr, false, false);
}

std::string CAtomicAPI::PostRequest(std::string strURL, jsoncons::json Data, SUserData* pUserData, bool bEncryptRequestBody, bool bDecryptRespnseBody)
{
    CURL*       curl;
    CURLcode    response_code;
    std::string response_buffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, strURL.c_str());

        auto request_body_buffer = bEncryptRequestBody
                                       ? SharedUtil::Base64Encode(Data.to_string().length() >= 16 ? g_pAtomicCore->Encrypt(Data.to_string()) : Data.to_string())
                                       : Data.to_string();

        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 5L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 10L);


        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body_buffer.c_str());

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "CashLine");

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, pUserData);

        response_code = curl_easy_perform(curl);
        if (response_code != CURLE_OK)
        {
            SharedUtil::AddDebugLog(skCrypt("curl_easy_perform() failed: %s (0x%x)"), curl_easy_strerror(response_code), response_code);
        }

        curl_easy_cleanup(curl);            // Cleanup cURL
    }

    if (bDecryptRespnseBody)
    {
        return g_pAtomicCore->Decrypt(SharedUtil::Base64Decode(response_buffer));
    }

    return response_buffer;
}

size_t CAtomicAPI::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

int CAtomicAPI::ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    if (dltotal > 0 && clientp != nullptr)
    {
        float fProgress = (float)dlnow / (float)dltotal * 100.0;
        reinterpret_cast<SUserData*>(clientp)->fProgress = fProgress;
    }
    return 0;
}