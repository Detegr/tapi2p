#ifndef TAPI2P_AES_H
#define TAPI2P_AES_H

#include "privatekey.h"
#include "publickey.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>

unsigned char* aes_encrypt_random_pass(unsigned char* data, int len, size_t pwlen, struct pubkey* key, size_t* enclen);
unsigned char* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, const char* keyname, size_t* enclen);
unsigned char* aes_decrypt(unsigned char* data, int len, const char* keyname, size_t *declen);
unsigned char* aes_decrypt_with_key(unsigned char* data, int len, struct privkey* privkey, size_t* declen);

#endif
