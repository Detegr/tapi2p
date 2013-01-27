#ifndef TAPI2P_KEYGENERATOR_H
#define TAPI2P_KEYGENERATOR_H

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/rand.h>
#include <string.h>

static const int T2PPRIVATEKEY=0x1;
static const int T2PPUBLICKEY=0x2;

int generate(const char* path, unsigned int keys);

#endif
