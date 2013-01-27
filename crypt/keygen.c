#include "keygen.h"
#include <limits.h>

static EVP_PKEY*			m_evpkey	=NULL;
static RSA*					m_rsa		=NULL;
static BIGNUM*				m_bignumber	=NULL;
static BIO*					m_bio		=NULL;
static PKCS8_PRIV_KEY_INFO*	m_p8info	=NULL;
static const int RSA_KEY_BITS=4096;

static int keygen_init(void);
static int keygen_bio_init(const char* path);

static int generate_privkey(void);
static int generate_pubkey(void);

int keygen_init(void)
{
	m_bignumber = BN_new();
	if(!m_bignumber)
	{
		fprintf(stderr, "Failed to init bignumber\n");
		return -1;
	}
	m_rsa=RSA_new();
	if(!m_rsa)
	{
		fprintf(stderr, "Failed to create RSA context\n");
		return -1;
	}
	if(!BN_set_word(m_bignumber, RSA_F4) || !RSA_generate_key_ex(m_rsa,RSA_KEY_BITS,m_bignumber,NULL))
	{
		fprintf(stderr, "Failed to generate RSA key\n");
		return -1;
	}
	m_evpkey=EVP_PKEY_new();
	if(!EVP_PKEY_set1_RSA(m_evpkey, m_rsa))
	{
		fprintf(stderr, "Unable to convert RSA key to EVP key\n");
		return -1;
	}
	m_p8info=EVP_PKEY2PKCS8(m_evpkey);
	if(!m_p8info)
	{
		fprintf(stderr, "Failed to convert EVP to PKCS8\n");
		return -1;
	}
	return 0;
}

void keygen_free(void)
{
	PKCS8_PRIV_KEY_INFO_free(m_p8info);
	EVP_PKEY_free(m_evpkey);
	BN_free(m_bignumber);
	RSA_free(m_rsa);
	BIO_free_all(m_bio);

	m_p8info=NULL;
	m_evpkey=NULL;
	m_bignumber=NULL;
	m_rsa=NULL;
	m_bio=NULL;
}

int keygen_bio_init(const char* path)
{
	if(!m_bio)
	{
		m_bio=BIO_new(BIO_s_file());
	}
	if(!m_bio)
	{
		fprintf(stderr, "Failed to create BIO.\n");
		return -1;
	}
	int ret=0;
	ret=BIO_write_filename(m_bio, (void*)path);
	if(ret<=0)
	{
		fprintf(stderr, "Failed to create file %s\n", path);
		return -1;
	}
	return 0;
}

int generate_privkey(void)
{
	if(!PEM_write_bio_PKCS8_PRIV_KEY_INFO(m_bio, m_p8info))
	{
		fprintf(stderr, "Failed to write private key\n");
		return -1;
	}
	return 0;
}

int generate_pubkey()
{
	if(!PEM_write_bio_RSA_PUBKEY(m_bio,m_rsa))
	{
		fprintf(stderr, "Failed to write public key\n");
		return -1;
	}
	return 0;
}

int generate(const char* path, unsigned int keys)
{
	int pathlen=strnlen(path, PATH_MAX);
	if(pathlen>PATH_MAX+4)
	{
		fprintf(stderr, "Path: %s is too long\n", path);
	}

	keygen_init();
	char path2[pathlen+5];
	memset(path2, 0, pathlen+5);
	memcpy(path2, path, pathlen);
	if(keys & T2PPUBLICKEY)
	{
		memcpy(path2+pathlen, ".pub", 4);
		if(keygen_bio_init(path2) != 0) return -1;
		if(generate_pubkey() != 0) return -1;
	}
	if(keys & T2PPRIVATEKEY)
	{
		if(keygen_bio_init(path) != 0) return -1;
		if(generate_privkey() != 0) return -1;
	}
	keygen_free();

	return 0;
}
