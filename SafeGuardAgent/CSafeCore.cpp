#include "CSafeCore.h"
#include "SharedUtil.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>

CSafeCore* g_pSafeCore = new CSafeCore();

std::vector<unsigned char> removePKCS7Padding(const std::vector<unsigned char>& paddedData)
{
    if (paddedData.empty())
    {
        throw std::runtime_error("Padded data is empty");
    }

    size_t paddingLength = paddedData.back();            // Last byte gives the length of the padding
    if (paddingLength > paddedData.size() || paddingLength == 0)
    {
        throw std::runtime_error("Invalid padding length");
    }

    // Check that all padding bytes are correct
    for (size_t i = 0; i < paddingLength; ++i)
    {
        if (paddedData[paddedData.size() - 1 - i] != paddingLength)
        {
            throw std::runtime_error("Invalid padding bytes");
        }
    }

    // Return the original data without the padding
    return std::vector<unsigned char>(paddedData.begin(), paddedData.end() - paddingLength);
}

CSafeCore::CSafeCore()
{
}

CSafeCore::~CSafeCore()
{
}

BYTE* CSafeCore::Encrypt(const BYTE* buffer)
{
    int ibuffer_size = strlen((char*)buffer);
    if (ibuffer_size == 0)
        return nullptr;

    const int& byte_key_index = SharedUtil::GenerateRandomNumber(0, m_vAESKeys.size() - 1);
    auto       key = m_vAESKeys[byte_key_index];
    auto       iv = m_vAESIVs[byte_key_index];

    AES_KEY encryption_key;
    if (AES_set_encrypt_key(key.data(), 256, &encryption_key) < 0)
        return nullptr;

    int                        icipher_text_size = ibuffer_size + AES_BLOCK_SIZE - (ibuffer_size % AES_BLOCK_SIZE);
    std::vector<unsigned char> vCipherText(icipher_text_size);

    AES_cbc_encrypt(buffer, vCipherText.data(), ibuffer_size, &encryption_key, iv.data(), AES_ENCRYPT);
    BYTE* out_buffer = vCipherText.data();
    return out_buffer;
}
void handleErrors()
{
    ERR_print_errors_fp(stderr);
    abort();
}

std::vector<unsigned char> hexToBytes(const std::string& hex)
{
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string   byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

bool aes_decrypt(const std::vector<unsigned char>& ciphertext, const unsigned char* key, const unsigned char* iv, std::vector<unsigned char>& plaintext)
{
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        handleErrors();
        return false;
    }

    int len;
    int plaintext_len = 0;

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        handleErrors();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 1);            // Ensure PKCS7 padding is expected

    plaintext.resize(ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()))
    {
        handleErrors();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len))
    {
        handleErrors();
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len += len;

    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);

    return true;
}

std::string unpad(const std::string& padded_data)
{
    if (padded_data.empty())
        return "";

    unsigned char pad_value = padded_data[padded_data.size() - 1];

    // Check that padding is valid
    size_t pad_length = static_cast<size_t>(pad_value);

    if (pad_length > padded_data.size() || pad_length > 16)
    {            // Max block size for AES is 16 bytes
        throw std::runtime_error("Invalid padding.");
    }

    return padded_data.substr(0, padded_data.size() - pad_length);
}

std::string CSafeCore::Decrypt(std::string strbuffer)
{
    return strbuffer;
    if (strbuffer.empty())
        return "";

    unsigned char ikey_index = static_cast<unsigned char>(strbuffer[0]) - 31;
    if (ikey_index >= m_vAESKeys.size() || ikey_index >= m_vAESIVs.size())
    {
        std::cerr << "Invalid key index!" << std::endl;
        return "";
    }

    const auto key = m_vAESKeys[ikey_index];
    const auto iv = m_vAESIVs[ikey_index];

    // Extract encrypted buffer
    std::string encrypted_buffer = strbuffer.substr(1);            // Skip first byte

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        throw std::runtime_error("Failed to create context.");

    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize decryption.");
    }

    int         len;
    std::string decrypted(encrypted_buffer.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()), '\0');

    // Decrypt
    if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(&decrypted[0]), &len, reinterpret_cast<const unsigned char*>(encrypted_buffer.data()),
                          encrypted_buffer.size()) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Decryption failed during update.");
    }

    int   outl;
    int plaintext_len = len;
    char* result = decrypted.data();

    // Finalize decryption
    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(result) + len, &outl) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        handleErrors();
    }

    plaintext_len += outl;

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return decrypted.substr(0, plaintext_len);            // Return only valid portion
}