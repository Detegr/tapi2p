#ifndef TAPI2P_PUBKEY_H
#define TAPI2P_PUBKEY_H

#include "key.h"

struct pubkey
{
	int m_encryptinit;
	struct key m_keydata;

	const char* keyastext;
};

void pubkey_init(struct pubkey* pkey);
void pubkey_free(struct pubkey* pkey);
int pubkey_load(struct pubkey* key, const char* file);
int pubkey_encrypt(struct pubkey* key, const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen);

#endif
