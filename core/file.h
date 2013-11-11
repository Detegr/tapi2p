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

void create_metadata_file(const char* from, char* file_sha_as_str);

#endif
