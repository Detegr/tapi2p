#ifndef TAPI2P_PRIVKEY_H
#define TAPI2P_KEY_H

#include "key.h"

struct privkey
{
	int m_decryptinit;
	struct key* m_keydata;
};

static int m_decryptinit(struct privkey* pkey);
void privkey_init(struct privkey* key);
int load(struct privkey* key, const char* file);
int decrypt(struct privkey* key, const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen);

#endif
