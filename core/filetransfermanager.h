#ifndef TAPI2P_FILETRANSFERMANAGER_H
#define TAPI2P_FILETRANSFERMANAGER_H

#include "peer.h"

typedef struct filetransferstatus
{
	const char *sha_str;
	uint32_t parts_ready;
	uint32_t part_count;
	uint64_t file_size;
} ftstatus;

void filetransfermanager_init(void);
void filetransfermanager_free(void);

int filetransfer_add(struct peer *p, const char *sha_str, metadata_t *md);
void filetransfer_part_ready(const char *sha_str, uint32_t partnum);
ftstatus filetransfer_get_status(const char *sha_str);
ftstatus *filetransfer_get_statuses(int *statuscount);

#endif
