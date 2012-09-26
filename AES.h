#pragma once

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

		static int M_DecryptInit(const unsigned char* magic, const unsigned char* salt, const unsigned char* pass);
	public:
		static void Initialize(const std::string& pass);
		static std::vector<unsigned char>& Encrypt(unsigned char* data, int len);
		static std::vector<unsigned char>& Decrypt(unsigned char* data, int len);
};
