#include "publickey.h"
#include <stdio.h>

static int m_encryptinit(struct pubkey* pkey);

void pubkey_init(struct pubkey* pkey)
{
	pkey->m_encryptinit=0;
	pkey->keyastext=NULL;
	key_init(&pkey->m_keydata);
}
void pubkey_free(struct pubkey* pkey)
{
	pkey->m_encryptinit=0;
	key_free(&pkey->m_keydata);
	if(pkey->keyastext)
	{
		free((char*)pkey->keyastext);
		pkey->keyastext=NULL;
	}
}

int pubkey_load(struct pubkey* pkey, const char* file)
{
	if(!pkey->m_keydata.m_ctx) pubkey_init(pkey);

	size_t size=0;
	FILE* f=fopen(file, "r");
	if(!f) return -1;
	fseek(f, 0, SEEK_END);
	size=ftell(f);
	fseek(f, 0, SEEK_SET);
	char* ktext;
	ktext = (char*)malloc(size*sizeof(char));
	fgets(ktext, size, f);
	fclose(f);

	pkey->keyastext=ktext;

	EVP_PKEY* evp_pkey;
	BIO_read_filename(pkey->m_keydata.m_bio, file);
	evp_pkey = PEM_read_bio_PUBKEY(pkey->m_keydata.m_bio, NULL, NULL, (void*)"");
	if(!evp_pkey)
	{
		fprintf(stderr, "Failed to load public key %s\n", file);
		return -1;
	}
	pkey->m_keydata.m_size=EVP_PKEY_size(evp_pkey);
	pkey->m_keydata.m_ctx=EVP_PKEY_CTX_new(evp_pkey, NULL);
	EVP_PKEY_free(evp_pkey);
	return 0;
}

static int m_encryptinit(struct pubkey* pkey)
{
	int ret=0;
	ret=EVP_PKEY_encrypt_init(pkey->m_keydata.m_ctx);
	if(ret<=0)
	{
		EVP_PKEY_CTX_free(pkey->m_keydata.m_ctx);
		return -1;
	}
	pkey->m_encryptinit=1;
	return 0;
}

int pubkey_encrypt(struct pubkey* pkey, const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen)
{
	if(!pkey)
	{
		fprintf(stderr, "No private key\n");
		return -1;
	}
	if(!pkey->m_keydata.m_ctx)
	{
		fprintf(stderr, "Cannot encrypt: No key loaded\n");
		return -1;
	}
	if(!pkey->m_encryptinit) m_encryptinit(pkey);

	int ret=0;
	ret=EVP_PKEY_encrypt(pkey->m_keydata.m_ctx, NULL, outlen, in, inlen);
	if(ret<=0)
	{
		fprintf(stderr, "Failed to get output buffer length\n");
		return -1;
	}
	else
	{
		*out=(unsigned char*)malloc(*outlen);
		if(!*out)
		{
			fprintf(stderr, "Failed to allocate output buffer\n");
			return -1;
		}
		memset(*out, 0, *outlen);
		ret=EVP_PKEY_encrypt(pkey->m_keydata.m_ctx, *out, outlen, in, inlen);
		if(ret<=0)
		{
			fprintf(stderr, "Failed to encrypt\n");
			return -1;
		}
	}
	return 0;
}
