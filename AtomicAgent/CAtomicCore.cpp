#include "CAtomicCore.h"
#include "SharedUtil.h"
#include "Helpers/AES.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>

CAtomicCore* g_pAtomicCore = new CAtomicCore();

std::string PadBuffer(const std::string& buffer)
{
    constexpr size_t blockSize = 16;
    size_t           paddingSize = blockSize - (buffer.size() % blockSize);
    std::string      paddedBuffer = buffer;
    paddedBuffer.append(paddingSize, static_cast<char>(paddingSize));
    return paddedBuffer;
}

std::string UnpadBuffer(std::string& input)
{
    if (input.empty())
        return "";

    // The padding length is stored in the last byte of the string (character).
    size_t pad_len = static_cast<size_t>(input.back());
    if (pad_len == 0 || pad_len > 16)
    {
        SharedUtil::AddDebugLog("Invalid padding length");
        return "";
    }

    // Check if the padding is valid
    for (size_t i = 0; i < pad_len; ++i)
    {
        if (input[input.size() - 1 - i] != static_cast<char>(pad_len))
        {
            SharedUtil::AddDebugLog("Invalid padding!");
            return "";
        }
    }

    // Create a new string without the padding
    return input.substr(0, input.size() - pad_len);
}

CAtomicCore::CAtomicCore()
{
}

CAtomicCore::~CAtomicCore()
{
}

std::string CAtomicCore::Encrypt(std::string buffer)
{
    int ibuffer_size = buffer.length();
    if (ibuffer_size == 0)
        return "";

    buffer = PadBuffer(buffer);

    const int&        byte_key_index = SharedUtil::GenerateRandomNumber(0, m_vAESKeys.size() - 1);
    std::vector<BYTE> key = m_vAESKeys[byte_key_index];
    std::vector<BYTE> iv = m_vAESIVs[byte_key_index];

    AES aes(AESKeyLength::AES_256);

    std::vector<BYTE> plaintext(buffer.begin(), buffer.end());
    std::vector<BYTE> out = aes.EncryptCBC(plaintext, key, iv);

    std::string outbuffer(out.begin(), out.end());
    outbuffer = (char)(byte_key_index + 31) + outbuffer;
    return outbuffer;
}

std::string CAtomicCore::Decrypt(std::string strbuffer)
{
    if (strbuffer.empty())
        return "";

    unsigned char ikey_index = static_cast<unsigned char>(strbuffer[0]) - 31;
    if (ikey_index >= m_vAESKeys.size() || ikey_index >= m_vAESIVs.size())
        return "";

    const auto key = m_vAESKeys[ikey_index];
    const auto iv = m_vAESIVs[ikey_index];

    // Extract encrypted buffer
    std::string buffer = strbuffer.substr(1);            // Skip first byte

    AES aes(AESKeyLength::AES_256);

    std::vector<BYTE> plaintext(buffer.begin(), buffer.end());
    std::vector<BYTE> out = aes.DecryptCBC(plaintext, key, iv);

    std::string outbuffer(out.begin(), out.end());
    return UnpadBuffer(outbuffer);
}