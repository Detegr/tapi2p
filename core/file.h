#ifndef TAPI2P_FILE_H
#define TAPI2P_FILE_H

#define FILE_PART_BYTES 524288 // 512kb file part size

#include <stdlib.h>

typedef struct file_transfer
{
	int sock;
	size_t part_count;
	size_t file_size;
	pthread_mutex_t file_lock;
} file_t;

void sha_to_str(const unsigned char* sha, char* out);
void create_metadata_file(const char* from, char* file_sha_as_str);
void check_or_create_metadata(const unsigned char* sha_data, size_t sha_size);

#endif
