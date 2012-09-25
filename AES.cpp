#include "AES.h"
#include <iostream>

EVP_CIPHER_CTX AES::m_Encrypt;
EVP_CIPHER_CTX AES::m_Decrypt;
std::vector<unsigned char> AES::m_EncData;
unsigned char AES::m_Salt[8];
std::string AES::m_Magic;

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
	if(!PKCS5_PBKDF2_HMAC_SHA1(pass.c_str(), pass.size(), m_Salt, PKCS5_SALT_LEN, AES::AES_ROUNDS, sizeof(buf), buf))
	{
		std::cerr << "err" << std::endl;
	}
	memcpy(key, buf, 32);
	memcpy(iv, buf+32, 16);

	EVP_CIPHER_CTX_init(&m_Encrypt);
	if(!EVP_EncryptInit_ex(&m_Encrypt, EVP_aes_256_cbc(), NULL, key, iv)) std::cerr << "Encryptinit failed" << std::endl;
	EVP_CIPHER_CTX_init(&m_Decrypt);
	if(!EVP_DecryptInit_ex(&m_Decrypt, EVP_aes_256_cbc(), NULL, key, iv)) std::cerr << "Decryptinit failed" << std::endl;

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
	int c_len=len+AES::AES_BLOCK_SIZE;
	int f_len=0;
	unsigned char c[c_len];

	EVP_EncryptInit_ex(&m_Encrypt, NULL, NULL, NULL, NULL);
	EVP_EncryptUpdate(&m_Encrypt, c, &c_len, data, len);
	EVP_EncryptFinal_ex(&m_Encrypt, c+c_len, &f_len);

	m_EncData.resize(m_Magic.size()+AES::AES_SALT_SIZE+c_len+f_len);
	memcpy(&m_EncData[0], m_Magic.c_str(), m_Magic.size());
	memcpy(&m_EncData[m_Magic.size()], m_Salt, AES::AES_SALT_SIZE);
	memcpy(&m_EncData[m_Magic.size()+AES::AES_SALT_SIZE], c, c_len+f_len);

	return m_EncData;
}

#include <fstream>

int main()
{
	AES::Initialize("password");
	std::vector<unsigned char>& encdata=AES::Encrypt((unsigned char*)"foobar\n", 7);
	std::ofstream f("out.enc");
	for(std::vector<unsigned char>::iterator it=encdata.begin(); it!=encdata.end(); ++it)
	{
		std::cout << (int)*it << " ";
		f << *it;
	}
	std::cout << std::endl;
	f.close();
}
