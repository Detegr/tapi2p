#include "filetransfermanager.h"
#include "file.h"
#include <assert.h>

#define MAX_PEERS_PER_TRANSFER 128 // TODO: Allow arbitrary amount of peers per filetransfer

typedef struct filetransferhelper
{
	char *sha_str;
	uint8_t *parts;

	struct filetransferstatus status;
} fthelper;

// TODO: Allow arbitrary amount of file transfers
static fthelper file_transfers[MAX_TRANSFERS];

void filetransfermanager_init(void)
{
	memset(file_transfers, 0, MAX_TRANSFERS*sizeof(fthelper));
}

void filetransfermanager_free(void)
{
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		if(file_transfers[i].sha_str)
		{
			free(file_transfers[i].sha_str);
			free(file_transfers[i].parts);
			memset(&file_transfers[i], 0, sizeof(fthelper));
		}
	}
}

int filetransfer_add(struct peer *p, const char *sha_str, metadata_t *md)
{
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		if(!file_transfers[i].sha_str)
		{
			file_transfers[i].parts=calloc(1, md->part_count*sizeof(uint8_t));
			file_transfers[i].status.parts_ready=0;
			file_transfers[i].status.part_count=md->part_count;
			strncpy(file_transfers[i].sha_str, sha_str, SHA_DIGEST_STR_MAX_LENGTH);
			file_transfers[i].status.sha_str=file_transfers[i].sha_str;
			return 0;
		}
	}
#ifndef NDEBUG
	fprintf(stderr, "No room for new file transfer!\n");
#endif
	return -1;
}

static fthelper *gethelper(const char *sha_str)
{
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		if(strncmp(file_transfers[i].sha_str, sha_str, SHA_DIGEST_STR_MAX_LENGTH) == 0)
		{
			return &file_transfers[i];
		}
	}
	return NULL;
}

void filetransfer_part_ready(const char *sha_str, uint32_t partnum)
{
	fthelper *h=gethelper(sha_str);
	if(!h) return;
	h->parts[partnum]=1;
	h->status.parts_ready++;
}

ftstatus filetransfer_get_status(const char *sha_str)
{
	fthelper *h=gethelper(sha_str);
	if(!h) assert(0);
	return h->status;
}

ftstatus *filetransfer_get_statuses(int *statuscount)
{
	int count=0;
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		if(file_transfers[i].sha_str) count++;
	}
	*statuscount=count;
	ftstatus *ret=malloc(count * sizeof(ftstatus));
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		if(file_transfers[i].sha_str)
		{
			ret[i]=file_transfers[i].status;
		}
	}
	return ret;
}
