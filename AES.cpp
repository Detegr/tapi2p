#include "AES.h"
#include <iostream>

EVP_CIPHER_CTX AES::m_Encrypt;
EVP_CIPHER_CTX AES::m_Decrypt;
std::vector<unsigned char> AES::m_EncData;
std::vector<unsigned char> AES::m_DecData;
unsigned char AES::m_Salt[8];
const char* AES::m_Magic;

void AES::Initialize(const std::string& pass)
{
	m_Magic="PBKDF2__";

	unsigned char buf[48];
	unsigned char key[32];
	unsigned char iv[16];

	if(!RAND_bytes(m_Salt, 8))
	{
		std::cerr << "PRNG _NOT_ SEEDED ENOUGH!!" << std::endl;
	}
	if(!PKCS5_PBKDF2_HMAC_SHA1(pass.c_str(), pass.size(), m_Salt, PKCS5_SALT_LEN, AES::ROUNDS, sizeof(buf), buf))
	{
		std::cerr << "err" << std::endl;
	}
	memcpy(key, buf, 32);
	memcpy(iv, buf+32, 16);

	EVP_CIPHER_CTX_init(&m_Encrypt);
	if(!EVP_EncryptInit_ex(&m_Encrypt, EVP_aes_256_cbc(), NULL, key, iv)) std::cerr << "Encryptinit failed" << std::endl;
	/*
	EVP_CIPHER_CTX_init(&m_Decrypt);
	if(!EVP_DecryptInit_ex(&m_Decrypt, EVP_aes_256_cbc(), NULL, key, iv)) std::cerr << "Decryptinit failed" << std::endl;
	*/

	std::cout << "key: ";
	for(int i=0; i<32; ++i)
	{
		std::cout << std::hex << (int)key[i] << "";
	}
	std::cout << std::endl;
	std::cout << "iv: ";
	for(int i=0; i<16; ++i)
	{
		std::cout << std::hex << (int)iv[i] << "";
	}
	std::cout << std::endl;
}

std::vector<unsigned char>& AES::Encrypt(unsigned char* data, int len)
{
	int c_len=len+AES::BLOCK_SIZE;
	int f_len=0;
	unsigned char c[c_len];

	EVP_EncryptInit_ex(&m_Encrypt, NULL, NULL, NULL, NULL);
	EVP_EncryptUpdate(&m_Encrypt, c, &c_len, data, len);
	EVP_EncryptFinal_ex(&m_Encrypt, c+c_len, &f_len);

	const char* pass="password";
	int passlen=9;
	m_EncData.resize(AES::MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)+passlen+c_len+f_len);
	memcpy(&m_EncData[0], m_Magic, AES::MAGIC_LEN);
	memcpy(&m_EncData[AES::MAGIC_LEN], m_Salt, PKCS5_SALT_LEN);
	memcpy(&m_EncData[AES::MAGIC_LEN+PKCS5_SALT_LEN], &passlen, sizeof(int));
	memcpy(&m_EncData[AES::MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)], pass, passlen);
	memcpy(&m_EncData[AES::MAGIC_LEN+PKCS5_SALT_LEN+sizeof(int)+passlen], c, c_len+f_len);

	return m_EncData;
}

int AES::M_DecryptInit(const unsigned char* const magic, const unsigned char* const salt, const unsigned char* const pass)
{
	unsigned char buf[48];
	unsigned char key[32];
	unsigned char iv[16];

	m_DecData.clear();
	if(memcmp(m_Magic, magic, AES::MAGIC_LEN) != 0)
	{
		std::cerr << "Invalid magic number" << std::endl;
		return -1;
	}

	int passlen=(*(const int*)pass);
	char ppass[passlen];
	memcpy(ppass, pass+sizeof(int), passlen);
	
	if(!PKCS5_PBKDF2_HMAC_SHA1(ppass, strnlen(ppass, 64), salt, PKCS5_SALT_LEN, AES::ROUNDS, sizeof(buf), buf))
	{
		std::cerr << "err" << std::endl;
	}
	memcpy(key, buf, 32);
	memcpy(iv, buf+32, 16);

	EVP_CIPHER_CTX_cleanup(&m_Decrypt);
	EVP_CIPHER_CTX_init(&m_Decrypt);
	if(!EVP_DecryptInit_ex(&m_Decrypt, EVP_aes_256_cbc(), NULL, key, iv)) std::cerr << "Decryptinit failed" << std::endl;

	return passlen+sizeof(int);
}

std::vector<unsigned char>& AES::Decrypt(unsigned char* data, int len)
{
	int passlen=M_DecryptInit(&data[0], &data[AES::MAGIC_LEN], &data[AES::MAGIC_LEN+PKCS5_SALT_LEN]);
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

#include <fstream>

int main()
{
	AES::Initialize("password");
	std::vector<unsigned char>& encdata=AES::Encrypt((unsigned char*)"foobar\n", 7);
	std::vector<unsigned char>& decdata=AES::Decrypt(&encdata[0], encdata.size());
	for(std::vector<unsigned char>::iterator it=decdata.begin(); it!=decdata.end(); ++it)
	{
		std::cout << (char)*it;
	}
}
