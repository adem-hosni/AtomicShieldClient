#include "CEagleCore.h"
#include "SharedUtil.h"
#include <stdexcept>

CEagleCore* g_pEagleCore = new CEagleCore();

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


CEagleCore::CEagleCore()
{
}

CEagleCore::~CEagleCore()
{
    
}

BYTE* CEagleCore::Encrypt(const BYTE* buffer)
{
    int ibuffer_size = strlen((char*)buffer);
    if (ibuffer_size == 0)
        return nullptr;

    const int& byte_key_index = SharedUtil::GenerateRandomNumber(0, m_vAESKeys.size()-1);
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

std::string CEagleCore::Decrypt(const BYTE* buffer)
{
    int ibuffer_size = strlen((char*)buffer + 1) - 1;
    int ikey_index = buffer[0] - 31;

    if (ibuffer_size < 4)
        return "";

    if (ikey_index> m_vAESIVs.size() || ikey_index > m_vAESKeys.size())
        return "";


    printf("selecting key %d\n", buffer[0] - 31);

    const std::vector<unsigned char>& key = m_vAESKeys[ikey_index];
    const std::vector<unsigned char>& iv = m_vAESIVs[ikey_index];


    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return "";

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1)
        return "";

    int ilen;
    int iplaintext_len;
    std::vector<BYTE> vdecrypted_data;
    vdecrypted_data.resize(ibuffer_size);

    if (EVP_DecryptUpdate(ctx, vdecrypted_data.data(), &ilen, buffer+1, ibuffer_size) != 1)
        return "";

    iplaintext_len = ilen;

    int r = EVP_DecryptFinal_ex(ctx, vdecrypted_data.data() + ilen, &ilen);
    if (r != 1)
    {
        return "";
    }

    iplaintext_len += ilen;
    vdecrypted_data.resize(iplaintext_len);
    EVP_CIPHER_CTX_free(ctx);

    auto vbuffer = removePKCS7Padding(vdecrypted_data);

    return (char*)vbuffer.data();
}