#include "PublicKey.h"
#include <iostream>
#include <fstream>

void RSA_PublicKey::Load(const std::string& file)
{
	std::string f=file+".pub";
	std::ifstream fi(f.c_str());
	if(fi.is_open())
	{
		std::string s;
		while(fi.good())
		{
			std::getline(fi,s);
			if(fi.good()) s+="\n";
			m_KeyAsText += s;
		}
		fi.close();
	}
	EVP_PKEY* pkey;
	BIO_read_filename(m_Bio, f.c_str());
	pkey = PEM_read_bio_PUBKEY(m_Bio, NULL, NULL, (void*)"");
	if(!pkey) throw KeyException("Failed to load public key: " + f);
	m_Size=EVP_PKEY_size(pkey);
	m_Ctx = EVP_PKEY_CTX_new(pkey, NULL);
	EVP_PKEY_free(pkey);
}

std::ostream& operator<<(std::ostream& o, const RSA_PublicKey& k)
{
	o << k.m_KeyAsText;
	return o;
}

void RSA_PublicKey::M_EncryptInit()
{
	int ret=0;
	ret=EVP_PKEY_encrypt_init(m_Ctx);
	if(ret<=0)
	{
		EVP_PKEY_CTX_free(m_Ctx);
		throw KeyException("Failed to initialize EVP encrypt");
	}
	m_EncryptInit=true;
}

void RSA_PublicKey::Encrypt(const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen)
{
	if(!m_Ctx) throw KeyException("Cannot encrypt: No key loaded");
	if(!m_EncryptInit) M_EncryptInit();

	int ret=0;
	ret=EVP_PKEY_encrypt(m_Ctx, NULL, outlen, in, inlen);
	if(ret<=0) throw KeyException("Failed to get output buffer length");
	else
	{
		*out=(unsigned char*)OPENSSL_malloc(*outlen);
		memset(*out, 0, *outlen);
		if(!*out) throw KeyException("Failed to allocate output buffer");
		ret=EVP_PKEY_encrypt(m_Ctx, *out, outlen, in, inlen);
		if(ret<=0) throw KeyException("Failed to encrypt");
	}
}
