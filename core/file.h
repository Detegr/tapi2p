#ifndef TAPI2P_FILE_H
#define TAPI2P_FILE_H

#define FILE_PART_BYTES 524288 // 512kB file part size
#define SHA_DIGEST_STR_MAX_LENGTH (SHA_DIGEST_LENGTH*2)+1

#include <stdlib.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>

struct peer;

typedef struct metadatareply
{
	uint32_t part_count;
	uint64_t file_size;
	uint8_t* data;
} metadata_t;

typedef struct file_transfer
{
	char sha_str[SHA_DIGEST_STR_MAX_LENGTH];
	FILE* file;
	pthread_mutex_t file_lock;
	metadata_t metadata;
} file_t;

typedef struct filepartrequest
{
	uint32_t part;
	int8_t sha_str[SHA_DIGEST_STR_MAX_LENGTH];
} fprequest_t;

typedef struct filepart
{
	int8_t sha_str[SHA_DIGEST_STR_MAX_LENGTH];
	uint32_t partnum;
	uint8_t* data;
} fp_t;

void sha_to_str(const unsigned char* sha, char* out);
void create_metadata_file(const char* from, char* file_sha_as_str);
void check_or_create_metadata(const unsigned char* sha_data, size_t sha_size);
void request_file_part_from_peer(int partnum, const char* sha_str, struct peer* p);
void request_file_part_listing_from_peers(const char *sha_str, struct peer *exclude_current);

unsigned char *SHA1_for_part(unsigned part, unsigned char *data, size_t datasize);

#endif
