#pragma once
#include "Key.h"

class RSA_PublicKey : public RSA_Key
{
	private:
		bool m_EncryptInit;
		void M_EncryptInit();
	public:
		RSA_PublicKey() : m_EncryptInit(false) {}
		void Load(const std::string& file);
		void Encrypt(const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen);
};
