#ifndef TAPI2P_PRIVKEY_H
#define TAPI2P_KEY_H

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>

struct key
{
	EVP_PKEY_CTX*				m_Ctx;
	BIO*						m_Bio;
	BIO*						m_BioErr;
	const unsigned char			m_Pad;
	int							m_Size;
};

void key_init(struct key* key);

#endif
