#ifndef TAPI2P_KEY_H
#define TAPI2P_KEY_H

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>

struct key
{
	EVP_PKEY_CTX*				m_ctx;
	BIO*						m_bio;
	BIO*						m_bioerr;
	const unsigned char			m_pad;
	int							m_size;
};

void key_init(struct key* key);
void key_free(struct key* key);

#endif
