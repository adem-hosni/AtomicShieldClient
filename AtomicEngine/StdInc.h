#pragma once
#include <string>
#include <sstream>
#include <unordered_set>
#include <variant>
#include <vector>
#include <eh.h>
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
#include <Psapi.h>
#include <iostream>
#include <filesystem>

#include "CCrashHandler.h"
#include "CAtomicHook.h"
#include "FileAuthentication.h"
#include "Screenshot.h"
#include "BasicChecks.h"
#include "AtomicPacket.h"
#include "CHWID.h"
#include "CAtomicNetwork.h"
#include "CAtomicThread.h"
#include "CAtomicAntiCheat.h"
#include "Common.h"
#include "SharedUtil.h"
#include "SharedProtocols.h"
#include "Utils.h"
#include "skCrypter.h"
// Guards
#include "Guards/CGuardBase.h"
#include "CGuardManager.h"
#include "Guards/CHeuristicGuard.h"
#include "Guards/CModuleGuard.h"
#include "Guards/CProcessGuard.h"
