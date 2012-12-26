#include "key.h"

void key_init(struct key* k)
{
	k->m_Pad=RSA_PKCS1_PADDING;
	k->m_Bio = BIO_new(BIO_s_file());
	k->m_BioErr = BIO_new_fp(stderr, BIO_NOCLOSE);
}

void key_init(struct key* k)
{

}
