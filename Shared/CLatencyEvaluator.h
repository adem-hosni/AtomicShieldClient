#pragma once
#include "CServerEndPoint.h"

class CLatencyEvaluator
{
public:
    void             AddServer(const std::string& url) { m_vServers.push_back(new CServerEndPoint(url)); }
    void             EvaluateAll();
    CServerEndPoint* GetBestServer();

    void        CacheEndPoint(std::string strEndPoint);
    std::string GetCachedEndPoint();

    static void SetupServerEndPoint();

private:
    std::vector<CServerEndPoint*> m_vServers;
};
