#include "CAtomicCore.h"
#include "SharedUtil.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>

CAtomicCore* g_pAtomicCore = new CAtomicCore();

CAtomicCore::CAtomicCore()
{
}

CAtomicCore::~CAtomicCore()
{
}

std::string padPlaintext(std::string& plaintext)
{
    size_t      padLength = 16 - (plaintext.size() % 16);
    std::string paddedText = plaintext;
    paddedText.resize(plaintext.size() + padLength, (BYTE)padLength);            // Pad with the length byte
    return paddedText;
}

std::string CAtomicCore::Encrypt(std::string buffer)
{
    return buffer;
    int ibuffer_size = buffer.length();
    if (ibuffer_size == 0)
        return "";

    const int& byte_key_index = SharedUtil::GenerateRandomNumber(0, m_vAESKeys.size() - 1);
    std::vector<BYTE> key = m_vAESKeys[byte_key_index];
    std::vector<BYTE> iv = m_vAESIVs[byte_key_index];

    BCRYPT_ALG_HANDLE hAlgorithm;
    BCRYPT_KEY_HANDLE hKey;
    NTSTATUS          status;
    DWORD             keyObjectSize = 0, resultSize = 0, ciphertextSize = 0;

    // Open AES algorithm provider
    status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to open AES algorithm provider", status);
    }

    // Get the key object size
    status = BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&keyObjectSize, sizeof(DWORD), &resultSize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to get key object size", status);
    }

    std::vector<BYTE> keyObject(keyObjectSize);

    // Create AES key
    status = BCryptGenerateSymmetricKey(hAlgorithm, &hKey, keyObject.data(), keyObjectSize, key.data(), key.size(), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to generate AES key", status);
    }

    // Calculate the ciphertext size
    status = BCryptEncrypt(hKey, reinterpret_cast<BYTE*>(buffer.data()), buffer.size(), NULL, const_cast<BYTE*>(iv.data()), iv.size(),
                           NULL, 0, &ciphertextSize, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to calculate ciphertext size (0x%x)", status);
    }

    std::vector<BYTE> ciphertext(ciphertextSize);

    // Perform encryption
    status = BCryptEncrypt(hKey, reinterpret_cast<BYTE*>(buffer.data()), buffer.size(), NULL, const_cast<BYTE*>(iv.data()), iv.size(),
                           ciphertext.data(), ciphertext.size(), &resultSize, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to encrypt plaintext (0x%x)", status);
    }

    // Cleanup
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlgorithm, 0);

    std::string result = reinterpret_cast<char*>(ciphertext.data());
    result = (char)(byte_key_index + 31) + result;
    return result;
}

std::string CAtomicCore::Decrypt(std::string strbuffer)
{
    return strbuffer;
    if (strbuffer.empty())
        return "";

    unsigned char ikey_index = static_cast<unsigned char>(strbuffer[0]) - 31;
    if (ikey_index >= m_vAESKeys.size() || ikey_index >= m_vAESIVs.size())
        return "";

    const auto key = m_vAESKeys[ikey_index];
    const auto iv = m_vAESIVs[ikey_index];

    // Extract encrypted buffer
    std::string encrypted_buffer = strbuffer.substr(1);            // Skip first byte

    BCRYPT_ALG_HANDLE hAlgorithm;
    BCRYPT_KEY_HANDLE hKey;
    NTSTATUS          status;
    DWORD             keyObjectSize = 0, resultSize = 0, plaintextSize = 0;

    // Open AES algorithm provider
    status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, NULL, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to open AES algorithm provider", status);
    }

    // Set the encryption mode to CBC explicitly (this is the default for AES)
    status = BCryptSetProperty(hAlgorithm, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        BCryptCloseAlgorithmProvider(hAlgorithm, 0);
        SharedUtil::AddDebugLog("Failed to set chaining mode to CBC.");
    }

    // Get the key object size
    status = BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH, (PUCHAR)&keyObjectSize, sizeof(DWORD), &resultSize, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to get key object size", status);
    }

    std::vector<BYTE> keyObject(keyObjectSize);

    // Create AES key
    status = BCryptGenerateSymmetricKey(hAlgorithm, &hKey, keyObject.data(), keyObjectSize, const_cast<BYTE*>(key.data()), key.size(), 0);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to generate AES key", status);
    }

    // Calculate the plaintext size
    status = BCryptDecrypt(hKey, reinterpret_cast<BYTE*>(encrypted_buffer.data()), encrypted_buffer.size(), NULL, const_cast<BYTE*>(iv.data()), iv.size(), NULL,
                           0, &plaintextSize, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to calculate plaintext size (0x%x)", status);
    }

    std::vector<BYTE> plaintext(plaintextSize);

    // Perform decryption
    status = BCryptDecrypt(hKey, reinterpret_cast<BYTE*>(encrypted_buffer.data()), encrypted_buffer.size(), NULL, const_cast<BYTE*>(iv.data()), iv.size(),
                           plaintext.data(), plaintext.size(), &resultSize, BCRYPT_BLOCK_PADDING);
    if (!BCRYPT_SUCCESS(status))
    {
        SharedUtil::AddDebugLog("Failed to decrypt ciphertext (0x%x)", status);
    }

    // Cleanup
    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlgorithm, 0);

    std::string decrypted_buffer = reinterpret_cast<char*>(plaintext.data());
    return decrypted_buffer;
}