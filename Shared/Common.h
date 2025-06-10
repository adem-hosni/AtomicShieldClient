#pragma once

#define PROJECT_NAME "Atomic Shield"

#define VERSION_MAJOR "1"
#define VERSION_MINOR "1"

#define VERSION_BETA

#ifdef VERSION_BETA
    #define PROJECT_VERSION VERSION_MAJOR "." VERSION_MINOR "-b"
#else
    #define PROJECT_VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR)
#endif

#define API_BASE_URL       "https://atomic-shield.com"
#define WEBSOCKET_BASE_URL "ws://157.173.212.241:443"

#define STRINGIFY(x) #x
