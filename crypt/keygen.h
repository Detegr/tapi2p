#ifndef TAPI2P_KEYGENERATOR_H
#define TAPI2P_KEYGENERATOR_H

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <string.h>

#define T2PPRIVATEKEY 0x1;
#define T2PPUBLICKEY 0x2;

static EVP_PKEY*			m_evpkey	=NULL;
static RSA*					m_rsa		=NULL;
static BIGNUM*				m_bignumber	=NULL;
static BIO*					m_bio		=NULL;
static PKCS8_PRIV_KEY_INFO*	m_p8info	=NULL;
static const int RSA_KEY_BITS=4096;

static int keygen_init();
static int keygen_bio_init(const char* path);

static int generate_privkey();
static int generate_pubkey();

int generate(const char* path, unsigned int keys);

#endif
