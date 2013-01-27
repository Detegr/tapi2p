#include "key.h"

void key_init(struct key* k)
{
	*(unsigned char*)&k->m_pad = RSA_PKCS1_PADDING;
	k->m_bio = BIO_new(BIO_s_file());
	k->m_bioerr = BIO_new_fp(stderr, BIO_NOCLOSE);
}

void key_free(struct key* k)
{
	BIO_free_all(k->m_bio);
	BIO_free_all(k->m_bioerr);
	if(k->m_ctx) EVP_PKEY_CTX_free(k->m_ctx);
	memset(k, 0, sizeof(struct key));
}
