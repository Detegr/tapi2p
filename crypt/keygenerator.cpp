#include "keygenerator.h"
#include "keyexception.h"

EVP_PKEY*				RSA_KeyGenerator::m_EvpKey;
RSA*					RSA_KeyGenerator::m_Rsa;
BIGNUM*					RSA_KeyGenerator::m_BigNumber;
BIO*					RSA_KeyGenerator::m_Bio;
PKCS8_PRIV_KEY_INFO*	RSA_KeyGenerator::m_P8Info;

void RSA_KeyGenerator::M_InitializeBio(const std::string& path)
{
	if(!m_Bio)
	{
		m_Bio=BIO_new(BIO_s_file());
	}
	if(!m_Bio) throw KeyException("Failed to create BIO.");
	int ret=0;
	ret=BIO_write_filename(m_Bio, (void*)path.c_str());
	if(ret<=0) throw KeyException("Failed to create file: " + path);
}

void RSA_KeyGenerator::M_Initialize()
{
	m_BigNumber=BN_new();
	if(!m_BigNumber) throw KeyException("Failed to create bignumber.");
	m_Rsa=RSA_new();
	if(!m_Rsa) throw KeyException("Failed to create RSA context");
	if(!BN_set_word(m_BigNumber, RSA_F4) || !RSA_generate_key_ex(m_Rsa,RSA_KEY_BITS,m_BigNumber,NULL)) throw KeyException("Failed to generate RSA key");
	m_EvpKey=EVP_PKEY_new();
	if(!EVP_PKEY_set1_RSA(m_EvpKey, m_Rsa)) throw KeyException("Unable to convert RSA key to EVP key");
	m_P8Info=EVP_PKEY2PKCS8(m_EvpKey);
	if(!m_P8Info) throw KeyException("Failed to convert EVP to PKCS8");
}

void RSA_KeyGenerator::M_WritePrivateKey()
{
	if(!PEM_write_bio_PKCS8_PRIV_KEY_INFO(m_Bio, m_P8Info)) throw KeyException("Failed to write private key");
}
void RSA_KeyGenerator::M_WritePublicKey()
{
	if(!PEM_write_bio_RSA_PUBKEY(m_Bio,m_Rsa)) throw KeyException("Failed to write public key");
}
void RSA_KeyGenerator::M_CloseBio()
{
}
void RSA_KeyGenerator::M_Close()
{
	PKCS8_PRIV_KEY_INFO_free(m_P8Info);
	EVP_PKEY_free(m_EvpKey);
	BN_free(m_BigNumber);
	RSA_free(m_Rsa);
	BIO_free_all(m_Bio);
}

void RSA_KeyGenerator::WriteKeyPair(const std::string& path)
{
	M_InitializeBio(path+".key");
	M_Initialize();
	M_WritePrivateKey();
	M_InitializeBio(path+".pub");
	M_WritePublicKey();
	M_Close();
}
