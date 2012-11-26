#pragma once
#include "key.h"

class RSA_PublicKey : public RSA_Key
{
	private:
		bool m_EncryptInit;
		void M_EncryptInit();
		std::string m_KeyAsText;
	public:
		RSA_PublicKey() : m_EncryptInit(false) {}
		void Print() const;
		void Load(const std::string& file);
		void Encrypt(const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen);
		friend std::ostream& operator<<(std::ostream& o, const RSA_PublicKey& k);
};
