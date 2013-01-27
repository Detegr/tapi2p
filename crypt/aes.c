#include "aes.h"
#include "publickey.h"
#include "privatekey.h"

#define ROUNDS 1
#define BLOCK_SIZE 16
#define MAGIC_LEN 8

static EVP_CIPHER_CTX	m_Encrypt;
static EVP_CIPHER_CTX	m_Decrypt;

static unsigned char	m_Salt[8];

static const char*		m_Magic="TAPI2P__";

static unsigned int		m_enclen=0;
static unsigned char*	m_encdata;
static unsigned int		m_declen=0;
static unsigned char*	m_decdata;

static int m_encryptinit(const char* pw, size_t pwlen);
static int m_decryptinit(const unsigned char* magic, const unsigned char* salt, const unsigned char* pass, struct privkey* privatekey);
static unsigned char* aes_encrypt_with_pass(unsigned char* data, int len, const char* pw, size_t pwlen, struct pubkey* pubkey, size_t* enclen);

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

unsigned char* aes_encrypt_random_pass(unsigned char* data, int len, size_t pwlen, struct pubkey* pubkey, size_t* enclen)
{
	unsigned char pw[pwlen];
	if(!RAND_bytes(pw, pwlen))
	{
		fprintf(stderr, "PRNG _NOT_ SEEDED ENOUGH!!\n");
	}
	return aes_encrypt_with_pass(data, len, (const char*)pw, pwlen, pubkey, enclen);
}

unsigned char* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, const char* keyname, size_t* enclen)
{
	struct pubkey pub;
	pubkey_init(&pub);
	pubkey_load(&pub, keyname);
	unsigned char* encdata=aes_encrypt_with_pass(data,len,pw,pwlen,&pub,enclen);
	pubkey_free(&pub);
	return encdata;
}

static unsigned char* aes_encrypt_with_pass(unsigned char* data, int len, const char* pw, size_t pwlen, struct pubkey* pubkey, size_t* enclen)
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
	size_t passlen;
	pubkey_encrypt(pubkey, (const unsigned char*)pw, pwlen, &encpass, &passlen);

	memset(m_encdata, 0, m_enclen);
	m_enclen=MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)+passlen+c_len+f_len;
	unsigned char* encdata=realloc(m_encdata, m_enclen);
	if(encdata) m_encdata=encdata;
	else
	{
		fprintf(stderr, "Failed to allocate memory for encrypting!\n");
		free(m_encdata);
		return NULL;
	}
	memcpy(&m_encdata[0], m_Magic, MAGIC_LEN);
	memcpy(&m_encdata[MAGIC_LEN], m_Salt, PKCS5_SALT_LEN);
	memcpy(&m_encdata[MAGIC_LEN+PKCS5_SALT_LEN], &passlen, sizeof(int));
	memcpy(&m_encdata[MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)], encpass, passlen);
	memcpy(&m_encdata[MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)+passlen], c, c_len+f_len);
	free(encpass);

	*enclen = m_enclen;
	return m_encdata;
}

int m_decryptinit(const unsigned char* const magic, const unsigned char* const salt, const unsigned char* const pass, struct privkey* privkey)
{
	unsigned char buf[48];
	unsigned char key[32];
	unsigned char iv[16];

	memset(m_decdata, 0, m_declen);
	if(memcmp(m_Magic, magic, MAGIC_LEN) != 0)
	{
		fprintf(stderr, "Invalid magic number\n");
		return -1;
	}

	int epasslen=(*(const int*)pass);
	char epass[epasslen];
	memcpy(epass, pass+sizeof(int), epasslen);
	
	char* plainpass;
	size_t plainpasslen;
	privkey_decrypt(privkey, (const unsigned char*)epass, epasslen, (unsigned char**)&plainpass, &plainpasslen);
	if(!PKCS5_PBKDF2_HMAC_SHA1(plainpass, plainpasslen, salt, PKCS5_SALT_LEN, ROUNDS, sizeof(buf), buf))
	{
		fprintf(stderr, "Key derivation failed!\n");
		return -1;
	}
	free(plainpass);

	memcpy(key, buf, 32);
	memcpy(iv, buf+32, 16);

	EVP_CIPHER_CTX_cleanup(&m_Decrypt);
	EVP_CIPHER_CTX_init(&m_Decrypt);
	if(!EVP_DecryptInit_ex(&m_Decrypt, EVP_aes_256_cbc(), NULL, key, iv))
	{
		fprintf(stderr, "Encryptinit failed!\n");
		return -1;
	}

	return epasslen+sizeof(int);
}

unsigned char* aes_decrypt(unsigned char* data, int len, const char* keyname, size_t* declen)
{
	struct privkey pk;
	privkey_init(&pk);
	privkey_load(&pk, keyname);
	unsigned char* decdata=aes_decrypt_with_key(data,len,&pk,declen);
	privkey_free(&pk);
	return decdata;
}

unsigned char* aes_decrypt_with_key(unsigned char* data, int len, struct privkey* privkey, size_t* declen)
{
	int passlen=m_decryptinit(&data[0], &data[MAGIC_LEN], &data[MAGIC_LEN+PKCS5_SALT_LEN], privkey);
	int llen=len-MAGIC_LEN-PKCS5_SALT_LEN-passlen;
	int p_len=llen;
	int f_len=0;
	unsigned char p[p_len+BLOCK_SIZE];

	EVP_DecryptInit_ex(&m_Decrypt, NULL, NULL, NULL, NULL);
	EVP_DecryptUpdate(&m_Decrypt, p, &p_len, &data[MAGIC_LEN+PKCS5_SALT_LEN+passlen], llen);
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
