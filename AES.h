#pragma once

#include "PrivateKey.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <vector>

class AES
{
	private:
		static const unsigned int			ROUNDS=1;
		static const unsigned int			BLOCK_SIZE=16;
		static const unsigned int			MAGIC_LEN=8;
		static EVP_CIPHER_CTX				m_Encrypt;
		static EVP_CIPHER_CTX				m_Decrypt;

		static std::vector<unsigned char>	m_EncData;
		static std::vector<unsigned char>	m_DecData;

		static unsigned char				m_Salt[8];

		static const char*					m_Magic;

		static void M_EncryptInit(const std::string& pass);
		static int M_DecryptInit(const unsigned char* magic, const unsigned char* salt, const unsigned char* pass, RSA_PrivateKey& privatekey);
	public:
		static std::vector<unsigned char>& Encrypt(unsigned char* data, int len, const std::string& password, const std::string& keyname);
		static std::vector<unsigned char>& Decrypt(unsigned char* data, int len, const std::string& keyname);
};
