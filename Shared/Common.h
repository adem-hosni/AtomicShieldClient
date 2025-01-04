#pragma once

#define PROJECT_NAME "Atomic Shield"

#define VERSION_MAJOR "1"
#define VERSION_MINOR "0"

#define VERSION_BETA

#ifdef VERSION_BETA
    #define PROJECT_VERSION VERSION_MAJOR "." VERSION_MINOR "-b"
#else
    #define PROJECT_VERSION STRINGIFY(VERSION_MAJOR) "." STRINGIFY(VERSION_MINOR)
#endif

// API Commons
// #define API_BASE_URL       "http://51.195.45.15"
// #define WEBSOCKET_BASE_URL "ws://51.195.45.15:8000"

#define API_BASE_URL       "http://127.0.0.1:8000"
#define WEBSOCKET_BASE_URL "ws://127.0.0.1:8000"

#define STRINGIFY(x) #x
