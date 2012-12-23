#pragma once

#include "privatekey.h"
#include "publickey.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <string>
#include <vector>

static const unsigned int			ROUNDS=1;
static const unsigned int			BLOCK_SIZE=16;
static EVP_CIPHER_CTX				m_Encrypt;
static EVP_CIPHER_CTX				m_Decrypt;

static std::vector<unsigned char>	m_EncData;
static std::vector<unsigned char>	m_DecData;

static unsigned char				m_Salt[8];

static const char*					m_Magic="TAPI2P__";

static int m_encryptinit(const char* pw, size_t pwlen);
static int m_decryptinit(const unsigned char* magic, const unsigned char* salt, const unsigned char* pass, rsa_privkey* privatekey);
static const unsigned int			MAGIC_LEN=8;

unsigned char* aes_encrypt(unsigned char* data, int len, size_t pwlen, struct rsa_pubkey* key);
unsigned char* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, const char* keyname);
unsigned char* aes_encrypt(unsigned char* data, int len, const char* pw, size_t pwlen, struct rsa_pubkey* pubkey);
unsigned char* aes_decrypt(unsigned char* data, int len, const char* keyname);
unsigned char* aes_decrypt(unsigned char* data, int len, struct rsa_privkey* key);
