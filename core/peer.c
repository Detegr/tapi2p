#include "peer.h"
#include "pathmanager.h"

void peer_init(struct peer* p)
{
	pubkey_init(&p->key);

	p->port=0;

	p->m_connectable=1;
	p->isock=-1;
	p->osock=-1;
	p->thread=0;
	p->m_key_ok=0;
	memset(p->file_transfers, 0, MAX_TRANSFERS*sizeof(file_t));
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		file_t* transfer=&p->file_transfers[i];
		transfer->metadata.file_size=0;
		transfer->metadata.part_count=0;
		transfer->metadata.data=NULL;
		pthread_mutex_init(&transfer->file_lock, NULL);
	}
}

void peer_free(struct peer* p)
{
	if(p->thread)
	{
		pthread_join(p->thread, NULL);
		p->thread=0;
	}
	pubkey_free(&p->key);
}

void clear_file_transfer(file_t* transfer)
{
	fclose(transfer->file);
	transfer->metadata.file_size=0;
	transfer->metadata.part_count=0;
	transfer->metadata.data=NULL;
	memset(transfer->sha_str, 0, SHA_DIGEST_LENGTH*2+1);
	transfer->file=NULL;
}

static void create_file(const char* filename, uint64_t filesize)
{
	FILE* f=fopen(filename, "r");
	if(!f)
	{
		perror("create_file");
		char c=0;
		f=fopen(filename, "w");
		fseek(f, filesize, SEEK_SET);
		// TODO: Not sure if this trick actually creates a sparse file correctly
		fwrite(&c, 0, 1, f);
	}
	fclose(f);
}

int create_file_transfer(struct peer* p, const char* sha_str, metadata_t* metadata)
{
	file_t* ft=get_and_lock_new_filetransfer(p);
	if(!ft)
	{
		printf("Failed to create file transfer, file will not be transferred!\n");
		return -1;
	}
	struct config* conf=getconfig();
	struct configitem* ci;
	if((ci=config_find_item(conf, "Filename", sha_str)) && ci->val)
	{
		strncpy(ft->sha_str, sha_str, SHA_DIGEST_LENGTH*2+1);
		memcpy(&ft->metadata, metadata, sizeof(metadata_t));
		printf("Creating file of size %lu\n", ft->metadata.file_size);
		create_file(ci->val, ft->metadata.file_size);
		ft->file=fopen(ci->val, "r+");
		if(!ft->file)
		{
			perror("Could not open created file");
		}
	}
	else
	{
		printf("Failed to create file for %s. No corresponding entry in config.\n", sha_str);
	}
	pthread_mutex_unlock(&ft->file_lock);
	return 0;
}

file_t* get_and_lock_new_filetransfer(struct peer* p)
{
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		if(pthread_mutex_trylock(&p->file_transfers[i].file_lock) == 0)
		{
			return &(p->file_transfers[i]);
		}
	}
	return NULL;
}

file_t* get_and_lock_existing_filetransfer_for_sha(struct peer* p, const char* sha_str)
{
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		if(strncmp(p->file_transfers[i].sha_str, sha_str, SHA_DIGEST_LENGTH*2+1) == 0)
		{
			if(pthread_mutex_trylock(&p->file_transfers[i].file_lock) == 0)
			{
				return &p->file_transfers[i];
			}
			else return NULL;
		}
	}
	return NULL;
}
