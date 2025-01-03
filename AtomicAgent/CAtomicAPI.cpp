#include "CAtomicAPI.h"
#include "CAtomicCore.h"

CAtomicAPI* g_pSafeAPI = new CAtomicAPI();

CAtomicAPI::CAtomicAPI()
{
}

CAtomicAPI::~CAtomicAPI()
{
}

jsoncons::json CAtomicAPI::GetStatus()
{
    std::string buffer = PostRequest(API_BASE_URL "/anticheat/status/agent", jsoncons::json(), false, false);
    if (buffer.empty())
    {
        jsoncons::json JsonResponse = jsoncons::json::object();
        JsonResponse["alive"] = false;
        JsonResponse["title"] = "Connection Error";
        JsonResponse["message"] = "Failed to connect to SafeGuard Master Server";

        return JsonResponse;
    }
    return jsoncons::json::parse(buffer);
}

void CAtomicAPI::DownloadEngine(std::string* buffer)
{
    *buffer = PostRequest(API_BASE_URL "/resources/scan/fivem");
}

std::string CAtomicAPI::PostRequest(const char* szURL, jsoncons::json Data, bool bEncryptRequestBody, bool bDecryptRespnseBody)
{
    CURL*       curl;
    CURLcode    response_code;
    std::string response_buffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, szURL);

        auto request_body_buffer = bEncryptRequestBody ? g_pAtomicCore->Encrypt((BYTE*)Data.to_string().c_str()) : (BYTE*)Data.to_string().c_str();

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body_buffer);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);

        response_code = curl_easy_perform(curl);
        if (response_code != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(response_code) << std::endl;
        }
    }

    if (bDecryptRespnseBody)
    {
        printf("Encrypted response size: %d\n", response_buffer.size());

        return response_buffer;
    }

    return response_buffer;
}

size_t CAtomicAPI::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}