#pragma once
#include "Key.h"

class RSA_PrivateKey : public RSA_Key
{
	private:
		bool m_DecryptInit;
		void M_DecryptInit();
	public:
		RSA_PrivateKey() : m_DecryptInit(false) {}
		void Load(const std::string& file);
		void Decrypt(const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen);
};
