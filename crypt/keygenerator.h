#pragma once

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <string>

class RSA_KeyGenerator
{
	private:
		static EVP_PKEY*			m_EvpKey;
		static RSA*					m_Rsa;
		static BIGNUM*				m_BigNumber;
		static BIO*					m_Bio;
		static PKCS8_PRIV_KEY_INFO*	m_P8Info;

		static void M_InitializeBio(const std::string& s);
		static void M_Initialize();
		static void M_WritePrivateKey();
		static void M_WritePublicKey();
		static void M_CloseBio();
		static void M_Close();
	public:
		static const int RSA_KEY_BITS=4096;
		static void WriteKeyPair(const std::string& path);
};
