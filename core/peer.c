#include "peer.h"

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
		pthread_mutex_init(&p->file_transfers[i].file_lock, NULL);
	}
}

void peer_free(struct peer* p)
{
	if(p->thread)
	{
		pthread_join(p->thread, NULL);
		p->thread=0;
	}
	/*
	if(p->key.m_keydata.m_ctx)
	{
		pubkey_free(&p->key);
	}
	*/
}

void clear_file_transfer(file_t* transfer)
{
	fclose(transfer->file);
	transfer->file=NULL;
	transfer->file_size=0;
	transfer->part_count=0;
	transfer->sock=0;
}
