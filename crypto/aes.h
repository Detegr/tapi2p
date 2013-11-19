#ifndef TAPI2P_AES_H
#define TAPI2P_AES_H

#include "privatekey.h"
#include "publickey.h"
#include "../core/event.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdint.h>


// Length of the password used to AES encrypt data
#define PW_LEN 80

typedef struct encrypted_data
{
	uint32_t m_Magic;
	uint8_t	 m_Salt[PKCS5_SALT_LEN];
	uint16_t m_PassLen;
	uint32_t m_DataLen;

	uint8_t* m_Data;
} enc_t;

enc_t* aes_encrypt_random_pass(unsigned char* data, int len, size_t pwlen, struct pubkey* pubkey, size_t* enclen);
enc_t* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, const char* keyname, size_t* enclen);
void* aes_decrypt(enc_t* data, const char* keyname, size_t *declen);
void* aes_decrypt_with_key(enc_t* data, struct privkey* privkey, size_t* declen);

void free_encdata(enc_t* enc);

#endif
