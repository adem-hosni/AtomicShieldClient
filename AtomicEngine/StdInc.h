#pragma once
#include <string>
#include <sstream>
#include <unordered_set>
#include <variant>
#include <vector>
// Third Parties
// Jsoncons
#include "jsoncons/json.hpp"

// IXWebSocket
#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <ixwebsocket/IXUserAgent.h>

#include <CRC32/CRC32.h>

#include <Windows.h>
#include <winternl.h>
#include <vector>
#include <Psapi.h>
#include <iostream>

#include "CAtomicHook.h"
#include "FileAuthentication.h"
#include "Screenshot.h"
#include "BasicChecks.h"
#include "AtomicPacket.h"
#include "CHWID.h"
#include "CAtomicNetwork.h"
#include "CAtomicThread.h"
#include "CAtomicAntiCheat.h"
#include "SharedUtil.h"
#include "SharedProtocols.h"
#include "Utils.h"

// Guards
#include "Guards/CGuardBase.h"
#include "CGuardManager.h"
#include "Guards/CMemoryGuard.h"
#include "Guards/CHeuristicGuard.h"
#include "Guards/CThreadGuard.h"
#include "Guards/CModuleGuard.h"
#include "Guards/CProcessGuard.h"
