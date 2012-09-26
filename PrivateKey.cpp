#include "PrivateKey.h"
#include <iostream>

void RSA_PrivateKey::Load(const std::string& file)
{
	std::string f=file+".key";
	EVP_PKEY* pkey;
	BIO_read_filename(m_Bio, f.c_str());
	pkey = PEM_read_bio_PrivateKey(m_Bio, NULL, NULL, (void*)"");
	if(!pkey) throw KeyException("Failed to load private key");
	m_Size=EVP_PKEY_size(pkey);
	m_Ctx = EVP_PKEY_CTX_new(pkey, NULL);
	EVP_PKEY_free(pkey);
}

void RSA_PrivateKey::M_DecryptInit()
{
	int ret=0;
	ret=EVP_PKEY_decrypt_init(m_Ctx);
	if(ret<=0)
	{
		EVP_PKEY_CTX_free(m_Ctx);
		throw KeyException("Failed to initialize EVP decrypt");
	}
	m_DecryptInit=true;

}

void RSA_PrivateKey::Decrypt(const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen)
{
	if(!m_Ctx) throw KeyException("Cannot decrypt: No key loaded");
	if(!m_DecryptInit) M_DecryptInit();

	int ret=0;
	ret=EVP_PKEY_decrypt(m_Ctx, NULL, outlen, in, inlen);
	if(ret<=0) throw KeyException("Failed to get output buffer length");
	else
	{
		*out=(unsigned char*)OPENSSL_malloc(*outlen);
		if(!*out) throw KeyException("Failed to allocate output buffer.");
		ret=EVP_PKEY_decrypt(m_Ctx, *out, outlen, in, inlen);
		if(ret<=0) throw KeyException("Failed to decrypt");
	}
}
