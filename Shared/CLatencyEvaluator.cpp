#include "skCrypter.h"
#include "CLatencyEvaluator.h"
#include "SharedUtil.h"
#include "CAtomicCore.h"
#include "Vendor/LiteRegedit/LiteRegedit.h"

#ifdef _ATOMIC_AGENT
    #include "CAtomicAPI.h"
#endif

#ifdef _ATOMIC_ENGINE
    #include "CAtomicNetwork.h"
#endif

void CLatencyEvaluator::EvaluateAll()
{
    SharedUtil::AddDebugLog("Evaluating server latencies (%d endpoints)...", m_vServers.size());
    for (auto& Server : m_vServers)
    {
        Server->MeasureLatency();
        SharedUtil::AddDebugLog("Server: %s, Latency: %ld ms", Server->GetUrl().c_str(), Server->GetLatencyMs());
    }
}

CServerEndPoint* CLatencyEvaluator::GetBestServer()
{
    CServerEndPoint* pBest = nullptr;
    for (auto& pServer : m_vServers)
    {
        if (pServer->GetLatencyMs() < (pBest ? pBest->GetLatencyMs() : (std::numeric_limits<long>::max)()))
        {
            pBest = pServer;
        }
    }
    return pBest;
}

void CLatencyEvaluator::CacheEndPoint(std::string strEndPoint)
{
    CLiteRegeditEasy* pRegistry = new CLiteRegeditEasy(HKEY_CURRENT_USER, "Software\\AtomicShield");

    std::string strEncrytedEndPoint = SharedUtil::Base64Encode(g_pAtomicCore->Encrypt(strEndPoint));
    
    pRegistry->WriteString("CachedEndPoint", strEncrytedEndPoint.c_str());
}

std::string CLatencyEvaluator::GetCachedEndPoint()
{
    CLiteRegeditEasy* pRegistry = new CLiteRegeditEasy(HKEY_CURRENT_USER, "Software\\AtomicShield");
    std::string       strEncryptedEndPoint = pRegistry->ReadString("CachedEndPoint");
    
    if (pRegistry->IsError() || strEncryptedEndPoint.length() < 5)
        return "";

    return g_pAtomicCore->Decrypt(SharedUtil::Base64Decode(strEncryptedEndPoint));
}


void CLatencyEvaluator::SetupServerEndPoint(void(__stdcall* EndPointCallback)(std::string), bool bUseCachedEndPoint)
{



    CLatencyEvaluator* pLatencyEvaluator = new CLatencyEvaluator();
    std::string        strCachedEndPoint = pLatencyEvaluator->GetCachedEndPoint();
    std::string        strBestEndPoint;
    if (!bUseCachedEndPoint || strCachedEndPoint.empty())
    {
        pLatencyEvaluator->AddServer("31.97.180.157");
        pLatencyEvaluator->AddServer("149.28.29.222");

        pLatencyEvaluator->EvaluateAll();

        CServerEndPoint* pBestServer = pLatencyEvaluator->GetBestServer();
        SharedUtil::AddDebugLog(skCrypt("Nearest Server: %s, Latency: %ld ms"), pBestServer->GetUrl().c_str(), pBestServer->GetLatencyMs());

        pLatencyEvaluator->CacheEndPoint(pBestServer->GetUrl());

        strBestEndPoint = pBestServer->GetUrl();
    }
    else
    {
        SharedUtil::AddDebugLog(skCrypt("Using cached endpoint: %s"), pLatencyEvaluator->GetCachedEndPoint().c_str());
        strBestEndPoint = strCachedEndPoint;
    }

    if (EndPointCallback)
        EndPointCallback("http://" + strBestEndPoint);

#ifdef _ATOMIC_AGENT
    g_pAtomicAPI->SetServerEndPoint(strBestEndPoint);
#endif

#ifdef _ATOMIC_ENGINE
    SharedUtil::AddDebugLog("Setting Server Endpoint: %s", strBestEndPoint.c_str());
    g_pAtomicNetwork->SetServerEndPoint(strBestEndPoint);
#endif
}