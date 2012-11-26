#include "key.h"

RSA_Key::RSA_Key()
{
	m_Bio = BIO_new(BIO_s_file());
	m_BioErr = BIO_new_fp(stderr, BIO_NOCLOSE);
}
RSA_Key::~RSA_Key()
{
}

int RSA_Key::Size() const { return m_Size; }
