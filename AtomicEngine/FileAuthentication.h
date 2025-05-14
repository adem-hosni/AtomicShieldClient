#pragma once
#include "StdInc.h"

#include <Softpub.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <mscat.h>
#include <wintrust.h>
#include <softpub.h>
#pragma comment(lib, "wintrust.lib")

namespace FileAuthentication
{
    bool VerifyEmbeddedSignature(LPCWSTR pwszSourceFile);
    bool VerifyCatalogSignature(LPCWSTR wszFilePath);
    bool HasSignature(LPCWSTR wszFilePath);
    bool IsFileSigned(const std::string& filePath);
};        
