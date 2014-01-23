#ifndef TAPI2P_FILE_H
#define TAPI2P_FILE_H

#define FILE_PART_BYTES 524288 // 512kB file part size

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
	char sha_str[SHA_DIGEST_LENGTH*2+1];
	FILE* file;
	pthread_mutex_t file_lock;
	metadata_t metadata;
} file_t;

typedef struct filepartrequest
{
	uint32_t part;
	int8_t sha_str[SHA_DIGEST_LENGTH*2+1];
} fprequest_t;

typedef struct filepart
{
	int8_t sha_str[SHA_DIGEST_LENGTH*2+1];
	uint32_t partnum;
	uint8_t* data;
} fp_t;

void sha_to_str(const unsigned char* sha, char* out);
void create_metadata_file(const char* from, char* file_sha_as_str);
void check_or_create_metadata(const unsigned char* sha_data, size_t sha_size);
void request_file_part_from_peer(int partnum, const char* sha_str, struct peer* p);

#endif