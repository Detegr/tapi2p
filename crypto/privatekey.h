#ifndef TAPI2P_PRIVKEY_H
#define TAPI2P_PRIVKEY_H

#include "key.h"

struct privkey
{
	int m_decryptinit;
	struct key m_keydata;
};

void privkey_init(struct privkey* pkey);
void privkey_free(struct privkey* pkey);
int privkey_load(struct privkey* key, const char* file);
int privkey_decrypt(struct privkey* key, const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen);

#endif
