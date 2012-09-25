#pragma once

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <string>
#include "KeyException.h"

class RSA_Key
{
	protected:
		EVP_PKEY_CTX*				m_Ctx;
		BIO*						m_Bio;
		BIO*						m_BioErr;
		static const unsigned char	m_Pad=RSA_PKCS1_PADDING;
		int							m_Size;
		RSA_Key();
		virtual ~RSA_Key();
	public:
		virtual void Load(const std::string& file)=0;
		virtual int Size() const;
};
