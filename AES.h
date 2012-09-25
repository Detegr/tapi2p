#pragma once

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <vector>

class AES
{
	private:
		static const unsigned int			AES_ROUNDS=1;
		static const unsigned int			AES_BLOCK_SIZE=16;
		static const unsigned int			AES_SALT_SIZE=8;
		static EVP_CIPHER_CTX				m_Encrypt;
		static EVP_CIPHER_CTX				m_Decrypt;

		static std::vector<unsigned char>	m_EncData;
		static std::vector<unsigned char>	m_DecData;

		static unsigned char				m_Salt[8];

		static std::string					m_Magic;
	public:
		static void Initialize(const std::string& pass);
		static std::vector<unsigned char>& Encrypt(unsigned char* data, int len);
};
