#include "handlers.h"
#include "core.h"
#include "peermanager.h"
#include "pathmanager.h"
#include "../dtgconf/src/config.h"
#include "util.h"
#include "../crypto/aes.h"

#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>

static void* sendmetadata(void* args)
{
	void** argarr=(void**)args;
	file_t* f=argarr[0];
	struct peer* p=argarr[1];
	char* filename=argarr[2];

	pthread_mutex_lock(&f->file_lock);

	FILE* md=fopen(filename, "r");
	if(md)
	{
		fseek(md, 0, SEEK_END);
		long size=ftell(md);
		fseek(md, 0, SEEK_SET);
		long sent=0;
		f->part_count=(size/FILE_PART_BYTES)+1;
		f->file_size=size;
		while(sent<size)
		{
			unsigned char buf[FILE_PART_BYTES];
			size_t read=fread(buf, FILE_PART_BYTES, 1, md);
			size_t enclen=0;
			// FIXME: NOT THREAD SAFE!
			unsigned char* data=aes_encrypt_random_pass(
								buf,
								read ? FILE_PART_BYTES : size,
								PW_LEN,
								&p->key,
								&enclen);
			ssize_t s=send(f->sock, data, enclen, 0);
			if(s<0)
			{
				perror("Send");
				f->part_count=0;
				f->file_size=0;
				return NULL;
			}
			sent+=read ? FILE_PART_BYTES : size;
			printf("Sent %lu bytes of encrypted data.\n", s); 
			printf("Sent %lu bytes of %lu\n", sent, size); 
		}
	}
	else
	{
		printf("Could not open metadata file\n");
	}
	pthread_mutex_unlock(&f->file_lock);

	free(filename);
	free(args);
	return 0;
}

void handlemessage(evt_t* e, void* data)
{
	send_to_all(e);
}

void handlelistpeers(evt_t* e, void* data)
{
	// TODO: If datalen>EVENT_DATALEN, strange things will happen...
	printf("ListPeers\n");
	struct config* c=(struct config*)data;
	char edata[EVENT_DATALEN];
	memset(edata, 0, sizeof(char)*EVENT_DATALEN);
	char* dp=edata;
	struct peer* p;
	while(p=peer_next())
	{
		struct configitem* ci=config_find_item(c, "Nick", p->addr);
		if(ci && ci->val)
		{
			dp=stpcpy(dp, ci->val);
			dp=stpcpy(dp, " ");
		}
		dp=stpcpy(dp, "[");
		dp=stpcpy(dp, p->addr);
		dp=stpcpy(dp, "]");
		if(!p->m_key_ok) dp=stpcpy(dp, " <Invalid key. No communication possible.>");
		else if(!(p->m_connectable && p->isock>0)) dp=stpcpy(dp, " <One-way>");
		dp=stpcpy(dp, "\n");
	}
	dp[-1]=0;
	printf("%s\n", edata);
	event_set(e, edata, strnlen(edata, EVENT_DATALEN));
	event_send(e, e->fd_from);
}

void handlefiletransferlocal(evt_t* e, void* data)
{
	struct peer* p;
	if((p=peer_exists_simple(e->addr, e->port)))
	{
		e->type=RequestFileTransfer;
		send_data_to_peer(p,e);
		printf("Sent HandleFileTransfer to: %s:%u\n", e->addr, e->port);
	}
	else
	{
		printf("Peer %s:%u not found.\n", e->addr, e->port);
	}
}

void handlefiletransfer(evt_t* e, void* data)
{
	struct config* conf=getconfig();
	const char* filehash=e->data;
	struct peer* p;
	struct configitem* ci;
	if((ci=config_find_item(conf, "Filename", filehash)) && ci->val)
	{
		if((p=peer_exists_simple(e->addr, e->port)))
		{
			FILE* f=fopen(ci->val, "r");
			if(f)
			{
				fclose(f);
				for(int i=0; i<64; ++i)
				{
					if(pthread_mutex_trylock(&p->file_transfers[i].file_lock) == 0 &&
						p->file_transfers[i].sock == 0)
					{
						p->file_transfers[i].sock=p->osock==SOCKET_ONEWAY?p->isock:p->osock;
						p->file_transfers[i].part_count=0;
						p->file_transfers[i].file_size=0;

						pthread_t sendthread;
						void** data=malloc(3*sizeof(int*));
						data[0]=&(p->file_transfers[i]);
						data[1]=p;
						data[2]=NULL;
						getpath(metadatapath(), filehash, (char**)&data[2]);
						pthread_create(&sendthread, NULL, &sendmetadata, data);

						pthread_mutex_unlock(&p->file_transfers[i].file_lock);
						break;
					}
				}
			}
			else
			{
#ifndef NDEBUG
				printf("Requested file not found\n");
#endif
			}
		}
		else
		{
			fprintf(stderr, "HandleFileTransfer: Peer not found\n");
		}
	}
	else
	{
#ifndef NDEBUG
		printf("No file name corresponding hash %s\n", filehash);
#endif
	}
}

void fileparthandler(evt_t* e, void* data)
{
	fprequest_t* req=(fprequest_t*)e->data;
	printf("File part %d requested for hash %s\n", req->part, req->sha_str);
}
