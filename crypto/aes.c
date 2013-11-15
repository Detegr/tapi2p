#include "aes.h"
#include "publickey.h"
#include "privatekey.h"

#define ROUNDS 1
#define BLOCK_SIZE 16
#define MAGIC_LEN 4

static EVP_CIPHER_CTX	m_Encrypt;
static EVP_CIPHER_CTX	m_Decrypt;

static unsigned char	m_Salt[8];

static uint32_t			m_Magic=0x0074B12B;

static unsigned int		m_enclen=0;
static unsigned char*	m_encdata;
static unsigned int		m_declen=0;
static unsigned char*	m_decdata;

static int m_encryptinit(const char* pw, size_t pwlen);
static void m_decryptinit(enc_t* data, struct privkey* privkey);
static enc_t* aes_encrypt_with_pass(unsigned char* data, int len, const char* pw, size_t pwlen, struct pubkey* pubkey, size_t* enclen);

static int m_encryptinit(const char* pw, size_t pwlen)
{
	unsigned char buf[48];
	unsigned char key[32];
	unsigned char iv[16];

	if(!RAND_bytes(m_Salt, 8))
	{
		fprintf(stderr, "PRNG _NOT_ SEEDED ENOUGH!!\n");
	}
	if(!PKCS5_PBKDF2_HMAC_SHA1(pw, pwlen, m_Salt, PKCS5_SALT_LEN, ROUNDS, sizeof(buf), buf)) return -1;
	memcpy(key, buf, 32);
	memcpy(iv, buf+32, 16);

	EVP_CIPHER_CTX_cleanup(&m_Encrypt);
	EVP_CIPHER_CTX_init(&m_Encrypt);
	if(!EVP_EncryptInit_ex(&m_Encrypt, EVP_aes_256_cbc(), NULL, key, iv)) return -1;

	return 0;
}

enc_t* aes_encrypt_random_pass(unsigned char* data, int len, size_t pwlen, struct pubkey* pubkey, size_t* enclen)
{
	unsigned char pw[pwlen];
	if(!RAND_bytes(pw, pwlen))
	{
		fprintf(stderr, "PRNG _NOT_ SEEDED ENOUGH!!\n");
	}
	return aes_encrypt_with_pass(data, len, (const char*)pw, pwlen, pubkey, enclen);
}

enc_t* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, const char* keyname, size_t* enclen)
{
	struct pubkey pub;
	pubkey_init(&pub);
	pubkey_load(&pub, keyname);
	enc_t* encdata=aes_encrypt_with_pass(data,len,pw,pwlen,&pub,enclen);
	pubkey_free(&pub);
	return encdata;
}

static enc_t* aes_encrypt_with_pass(unsigned char* data, int len, const char* pw, size_t pwlen, struct pubkey* pubkey, size_t* enclen)
{
	m_encryptinit(pw, pwlen);

	int c_len=len+BLOCK_SIZE;
	int f_len=0;
	unsigned char c[c_len];

	EVP_EncryptInit_ex(&m_Encrypt, NULL, NULL, NULL, NULL);
	EVP_EncryptUpdate(&m_Encrypt, c, &c_len, data, len);
	EVP_EncryptFinal_ex(&m_Encrypt, c+c_len, &f_len);
	EVP_CIPHER_CTX_cleanup(&m_Encrypt);

	unsigned char* encpass;
	size_t encpasslen;
	pubkey_encrypt(pubkey, (const unsigned char*)pw, pwlen, &encpass, &encpasslen);
	
	size_t elen=sizeof(enc_t)+encpasslen+c_len+f_len;

	enc_t* enc=malloc(elen);
	enc->m_Magic=m_Magic;
	memcpy(enc->m_Salt, m_Salt, PKCS5_SALT_LEN);
	enc->m_PassLen=encpasslen;
	enc->m_DataLen=c_len+f_len;
	enc->m_Data=(uint8_t*)enc + sizeof(enc_t);
	memcpy(&enc->m_Data[0], encpass, enc->m_PassLen);
	memcpy(&enc->m_Data[enc->m_PassLen], c, c_len+f_len);

	free(encpass);

	*enclen = elen;
	return enc;
}

void m_decryptinit(enc_t* data, struct privkey* privkey)
{
	unsigned char buf[48];
	unsigned char key[32];
	unsigned char iv[16];

	memset(m_decdata, 0, m_declen);
	if(data->m_Magic != m_Magic)
	{
		fprintf(stderr, "Invalid magic number\n");
		return;
	}

	char* plainpass;
	size_t plainpasslen;
	privkey_decrypt(privkey, data->m_Data, data->m_PassLen, (unsigned char**)&plainpass, &plainpasslen);
	if(!PKCS5_PBKDF2_HMAC_SHA1(plainpass, plainpasslen, data->m_Salt, PKCS5_SALT_LEN, ROUNDS, sizeof(buf), buf))
	{
		fprintf(stderr, "Key derivation failed!\n");
		return;
	}
	free(plainpass);

	memcpy(key, buf, 32);
	memcpy(iv, buf+32, 16);

	EVP_CIPHER_CTX_cleanup(&m_Decrypt);
	EVP_CIPHER_CTX_init(&m_Decrypt);
	if(!EVP_DecryptInit_ex(&m_Decrypt, EVP_aes_256_cbc(), NULL, key, iv))
	{
		fprintf(stderr, "Encryptinit failed!\n");
		return;
	}
}

unsigned char* aes_decrypt(enc_t* data, const char* keyname, size_t* declen)
{
	struct privkey pk;
	privkey_init(&pk);
	privkey_load(&pk, keyname);
	unsigned char* decdata=aes_decrypt_with_key(data,&pk,declen);
	privkey_free(&pk);
	return decdata;
}

unsigned char* aes_decrypt_with_key(enc_t* data, struct privkey* privkey, size_t* declen)
{
	m_decryptinit(data, privkey);
	int llen=data->m_DataLen;
	int p_len=llen;
	int f_len=0;
	unsigned char p[p_len+BLOCK_SIZE];

	EVP_DecryptInit_ex(&m_Decrypt, NULL, NULL, NULL, NULL);
	EVP_DecryptUpdate(&m_Decrypt, p, &p_len, data->m_Data+data->m_PassLen, llen);
	EVP_DecryptFinal_ex(&m_Decrypt, p+p_len, &f_len);
	EVP_CIPHER_CTX_cleanup(&m_Decrypt);

	memset(m_decdata, 0, m_declen);
	unsigned char* decdata=(unsigned char*)realloc(m_decdata, p_len+f_len);
	if(decdata)
	{
		m_decdata=decdata;
	}
	else
	{
		fprintf(stderr, "Failed to allocate memory for decrypting!\n");
		free(m_decdata);
		return NULL;
	}
	m_declen=p_len+f_len;
	memcpy(&m_decdata[0], p, p_len+f_len);

	*declen=m_declen;
	return m_decdata;
}
