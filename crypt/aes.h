#pragma once

#include "privatekey.h"
#include "publickey.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string.h>

#define ROUNDS 1
#define BLOCK_SIZE 16
#define MAGIC_LEN 8

static EVP_CIPHER_CTX				m_Encrypt;
static EVP_CIPHER_CTX				m_Decrypt;

static unsigned char				m_Salt[8];

static const char*					m_Magic="TAPI2P__";

unsigned int						m_enclen=0;
unsigned char*						m_encdata;
unsigned int						m_declen=0;
unsigned char*						m_decdata;

static int m_encryptinit(const char* pw, size_t pwlen);
static int m_decryptinit(const unsigned char* magic, const unsigned char* salt, const unsigned char* pass, struct privkey* privatekey);

static unsigned char* aes_encrypt_random_pass(unsigned char* data, int len, size_t pwlen, struct pubkey* key);
static unsigned char* aes_encrypt_with_pass(unsigned char* data, int len, const char* pw, size_t pwlen, struct pubkey* pubkey);
unsigned char* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, const char* keyname);
unsigned char* aes_decrypt(unsigned char* data, int len, const char* keyname);
static unsigned char* aes_decrypt_with_key(unsigned char* data, int len, struct privkey* key);
