#include "privatekey.h"
#include <stdio.h>

void privkey_init(struct privkey* pkey)
{
	pkey->m_decryptinit=0;
	key_init(pkey->m_keydata);
}

int load(struct privkey* pkey, const char* file)
{
	EVP_PKEY* evp_pkey;
	BIO_read_filename(pkey->m_keydata->m_Bio, file);
	evp_pkey = PEM_read_bio_PrivateKey(pkey->m_keydata->m_Bio, NULL, NULL, (void*)"");
	if(!evp_pkey) return -1;//throw KeyException("Failed to load private key");
	pkey->m_keydata->m_Size=EVP_PKEY_size(evp_pkey);
	pkey->m_keydata->m_Ctx = EVP_PKEY_CTX_new(evp_pkey, NULL);
	EVP_PKEY_free(evp_pkey);
}

static int m_decryptinit(struct privkey* pkey)
{
	int ret=0;
	ret=EVP_PKEY_decrypt_init(pkey->m_keydata->m_Ctx);
	if(ret<=0)
	{
		EVP_PKEY_CTX_free(pkey->m_keydata->m_Ctx);
		return -1;
		//throw KeyException("Failed to initialize EVP decrypt");
	}
	pkey->m_decryptinit=1;
}

int decrypt(struct privkey* pkey, const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen)
{
	if(!pkey->m_decryptinit) m_decryptinit(pkey);

	int ret=0;
	ret=EVP_PKEY_decrypt(pkey->m_keydata->m_Ctx, NULL, outlen, in, inlen);
	if(ret<=0) return -1;//throw KeyException("Failed to get output buffer length");
	else
	{
		*out=(unsigned char*)OPENSSL_malloc(*outlen);
		if(!*out) return -2;//throw KeyException("Failed to allocate output buffer.");
		ret=EVP_PKEY_decrypt(pkey->m_keydata->m_Ctx, *out, outlen, in, inlen);
		if(ret<=0) return -3;//throw KeyException("Failed to decrypt");
	}
}
