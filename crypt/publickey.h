#pragma once
#include "key.h"

struct pubkey
{
	const char* m_keyastext;
	int m_decryptinit;
	struct key* m_keydata;
};

static int m_decryptinit(struct pubkey* pkey);
void pubkey_init(struct pubkey* key);
int load(struct pubkey* key, const char* file);
int encrypt(struct pubkey* key, const unsigned char* in, size_t inlen, unsigned char** out, size_t* outlen);
