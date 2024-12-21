#pragma once
#include <string>

// Third Parties
// Jsoncons
#include "jsoncons/json.hpp"

// IXWebSocket
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>

#include <CRC32/CRC32.h>

#include "SafePacketID.h"
#include "CHWID.h"
#include "CMemoryScanner.h"
#include "CSafeNetwork.h"
#include "CAtomicThread.h"
#include "CSafeAntiCheat.h"
#include "SharedUtil.h"
#include "Utils.h"

// Guards
#include "Guards/CGuardBase.h"
#include "CGuardManager.h"
#include "Guards/CMemoryGuard.h"
#include "Guards/CHeuristicGuard.h"
#include "Guards/CThreadGuard.h"
#include "Guards/CModuleGuard.h"
