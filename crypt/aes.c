#include "aes.h"
#include <iostream>
#include "keyexception.h"
#include "publickey.h"
#include "privatekey.h"

int m_encryptInit(const char* pw, size_t pwlen)
{
	unsigned char buf[48];
	unsigned char key[32];
	unsigned char iv[16];

	if(!RAND_bytes(m_Salt, 8))
	{
		fprintf(stderr, "PRNG _NOT_ SEEDED ENOUGH!!\n");
	}
	if(!PKCS5_PBKDF2_HMAC_SHA1(pw, pwlen, m_Salt, PKCS5_SALT_LEN, :ROUNDS, sizeof(buf), buf)) return -1;
	memcpy(key, buf, 32);
	memcpy(iv, buf+32, 16);

	EVP_CIPHER_CTX_cleanup(&m_Encrypt);
	EVP_CIPHER_CTX_init(&m_Encrypt);
	if(!EVP_EncryptInit_ex(&m_Encrypt, EVP_aes_256_cbc(), NULL, key, iv)) return -1;
}

unsigned char* aes_encrypt(unsigned char* data, int len, size_t pwlen, struct rsa_pubkey* pubkey)
{
	unsigned char pw[pwlen];
	if(!RAND_bytes(pw, pwlen))
	{
		fprintf(stderr, "PRNG _NOT_ SEEDED ENOUGH!!\n");
	}
	return Encrypt(data, len, (const char*)pw, pwlen, pubkey);
}

unsigned char* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, const char* keyname)
{
	RSA_PublicKey pubkey;
	pubkey.Load(keyname);
	return AES::Encrypt(data,len,pw,pwlen,pubkey);
}

unsigned char* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, struct rsa_pubkey* pubkey)
{
	M_EncryptInit(pw, pwlen);

	int c_len=len+BLOCK_SIZE;
	int f_len=0;
	unsigned char c[c_len];

	EVP_EncryptInit_ex(&m_Encrypt, NULL, NULL, NULL, NULL);
	EVP_EncryptUpdate(&m_Encrypt, c, &c_len, data, len);
	EVP_EncryptFinal_ex(&m_Encrypt, c+c_len, &f_len);
	EVP_CIPHER_CTX_cleanup(&m_Encrypt);

	unsigned char* encpass;
	size_t passlen;
	pubkey.Encrypt((const unsigned char*)pw, pwlen, &encpass, &passlen);

	m_EncData.resize(AES::MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)+passlen+c_len+f_len);
	memcpy(&m_EncData[0], m_Magic, AES::MAGIC_LEN);
	memcpy(&m_EncData[AES::MAGIC_LEN], m_Salt, PKCS5_SALT_LEN);
	memcpy(&m_EncData[AES::MAGIC_LEN+PKCS5_SALT_LEN], &passlen, sizeof(int));
	memcpy(&m_EncData[AES::MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)], encpass, passlen);
	memcpy(&m_EncData[AES::MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)+passlen], c, c_len+f_len);
	OPENSSL_free(encpass);

	return m_EncData;
}

int m_decryptinit(const unsigned char* const magic, const unsigned char* const salt, const unsigned char* const pass, struct rsa_privkey* privkey)
{
	unsigned char buf[48];
	unsigned char key[32];
	unsigned char iv[16];

	m_DecData.clear();
	if(memcmp(m_Magic, magic, MAGIC_LEN) != 0)
	{
		std::cerr << "Invalid magic number" << std::endl;
		return -1;
	}

	int epasslen=(*(const int*)pass);
	char epass[epasslen];
	memcpy(epass, pass+sizeof(int), epasslen);
	
	char* plainpass;
	size_t plainpasslen;
	privkey.Decrypt((const unsigned char*)epass, epasslen, (unsigned char**)&plainpass, &plainpasslen);
	if(!PKCS5_PBKDF2_HMAC_SHA1(plainpass, plainpasslen, salt, PKCS5_SALT_LEN, ROUNDS, sizeof(buf), buf)) return -1; //throw KeyException("Key derivation failed");
	OPENSSL_free(plainpass);

	memcpy(key, buf, 32);
	memcpy(iv, buf+32, 16);

	EVP_CIPHER_CTX_cleanup(&m_Decrypt);
	EVP_CIPHER_CTX_init(&m_Decrypt);
	if(!EVP_DecryptInit_ex(&m_Decrypt, EVP_aes_256_cbc(), NULL, key, iv)) return -1; //throw KeyException("Encryptinit failed");

	return epasslen+sizeof(int);
}

unsigned char* aes_decrypt(unsigned char* data, int len, const char* keyname)
{
	RSA_PrivateKey pk;
	pk.Load(keyname);
	return aes_decrypt(data,len,pk);
}

unsigned char* aes_decrypt(unsigned char* data, int len, struct rsa_privkey* privkey)
{
	int passlen=m_decryptinit(&data[0], &data[AES::MAGIC_LEN], &data[AES::MAGIC_LEN+PKCS5_SALT_LEN], privkey);
	int llen=len-AES::MAGIC_LEN-PKCS5_SALT_LEN-passlen;
	int p_len=llen;
	int f_len=0;
	unsigned char p[p_len+AES::BLOCK_SIZE];

	EVP_DecryptInit_ex(&m_Decrypt, NULL, NULL, NULL, NULL);
	EVP_DecryptUpdate(&m_Decrypt, p, &p_len, &data[AES::MAGIC_LEN+PKCS5_SALT_LEN+passlen], llen);
	EVP_DecryptFinal_ex(&m_Decrypt, p+p_len, &f_len);
	EVP_CIPHER_CTX_cleanup(&m_Decrypt);

	m_DecData.clear();
	m_DecData.resize(p_len+f_len);
	memcpy(&m_DecData[0], p, p_len+f_len);

	return m_DecData;
}
/*
#include <fstream>

int main()
{
	try
	{
		std::vector<unsigned char>& encdata=AES::Encrypt((unsigned char*)"foobar\n", 7, "password", "key");
		for(std::vector<unsigned char>::iterator it=encdata.begin(); it!=encdata.end(); ++it)
		{
			std::cout << std::hex << (unsigned char)*it;
		}
		std::cout << std::endl;
		std::vector<unsigned char>& decdata=AES::Decrypt(&encdata[0], encdata.size(), "key");
		for(std::vector<unsigned char>::iterator it=decdata.begin(); it!=decdata.end(); ++it)
		{
			std::cout << (char)*it;
		}
	} catch(KeyException& e)
	{
		std::cerr << e.what() << std::endl;
	}
}
*/
