#include "privatekey.h"
#include <stdio.h>

void privkey_init(struct privkey* pkey)
{
	pkey->m_decryptinit=0;
	key_init(&pkey->m_keydata);
}
void privkey_free(struct privkey* pkey)
{
	pkey->m_decryptinit=0;
	key_free(&pkey->m_keydata);
}

int privkey_load(struct privkey* pkey, const char* file)
{
	if(!pkey->m_keydata.m_ctx) privkey_init(pkey);
	EVP_PKEY* evp_pkey;
	BIO_read_filename(pkey->m_keydata.m_bio, file);
	evp_pkey = PEM_read_bio_PrivateKey(pkey->m_keydata.m_bio, NULL, NULL, (void*)"");
	if(!evp_pkey)
	{
		fprintf(stderr, "Failed to load private key\n");
		return -1;
	}
	pkey->m_keydata.m_size=EVP_PKEY_size(evp_pkey);
	pkey->m_keydata.m_ctx = EVP_PKEY_CTX_new(evp_pkey, NULL);
	EVP_PKEY_free(evp_pkey);
	return 0;
}

static int m_privkey_decryptinit(struct privkey* pkey)
{
	int ret=0;
	ret=EVP_PKEY_decrypt_init(pkey->m_keydata.m_ctx);
	if(ret<=0)
	{
		fprintf(stderr, "Failed to initialize EVP decrypt\n");
		EVP_PKEY_CTX_free(pkey->m_keydata.m_ctx);
		return -1;
	}
	pkey->m_decryptinit=1;
	return 0;
}

int privkey_decrypt(struct privkey* pkey, const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen)
{
	if(!pkey->m_decryptinit) m_privkey_decryptinit(pkey);

	int ret=0;
	ret=EVP_PKEY_decrypt(pkey->m_keydata.m_ctx, NULL, outlen, in, inlen);
	if(ret<=0)
	{
		fprintf(stderr, "Failed to get output buffer length\n");
		return -1;
	}
	else
	{
		*out=(unsigned char*)OPENSSL_malloc(*outlen);
		if(!*out)
		{
			fprintf(stderr, "Failed to allocate output buffer\n");
			return -1;
		}
		ret=EVP_PKEY_decrypt(pkey->m_keydata.m_ctx, *out, outlen, in, inlen);
		if(ret<=0)
		{
			fprintf(stderr, "Failed to decrypt\n");
			return -1;
		}
	}
	return 0;
}
