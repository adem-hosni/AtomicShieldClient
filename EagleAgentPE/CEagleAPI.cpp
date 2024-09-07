#include "CEagleAPI.h"
#include "CEagleCore.h"

CEagleAPI* g_pEagleAPI = new CEagleAPI();

CEagleAPI::CEagleAPI()
{
}

CEagleAPI::~CEagleAPI()
{
}

void CEagleAPI::DownloadAgentPEB(bool* bComplete)
{
    std::string peb_buffer = PostRequest("http://127.0.0.1:8000/resources/agentpeb");
    printf("agent peb buffer size: %d\n", peb_buffer.size());
    *bComplete = true;
}

std::string CEagleAPI::PostRequest(const char* szURL, jsoncons::json Data, bool bEncryptRequestBody, bool bDecryptRespnseBody)
{
    CURL* curl;
    CURLcode response_code;
    std::string response_buffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, szURL);


        auto request_body_buffer = bEncryptRequestBody ? g_pEagleCore->Encrypt((BYTE*)Data.to_string().c_str()) : (BYTE*)Data.to_string().c_str();

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

size_t CEagleAPI::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}