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

struct file_part_thread_data
{
	uint32_t partnum;
	struct peer* peer;
	file_t* transfer;
};

static ssize_t send_file_part(struct file_part_thread_data* td)
{
	unsigned char buf[FILE_PART_BYTES];
	size_t read=fread(buf+(td->partnum*FILE_PART_BYTES), FILE_PART_BYTES, 1, td->transfer->file);
	size_t enclen=0;

	if(td->transfer->file_size==0)
	{
		fseek(td->transfer->file, 0, SEEK_END);
		td->transfer->file_size=ftell(td->transfer->file);
		printf("File size: %lu\n", td->transfer->file_size);
		fseek(td->transfer->file, 0, SEEK_SET);
		td->transfer->part_count=(td->transfer->file_size/FILE_PART_BYTES)+1;
	}

	unsigned char* data=aes_encrypt_random_pass(
						buf,
						read ? FILE_PART_BYTES : (td->transfer->file_size)-(td->partnum*FILE_PART_BYTES),
						PW_LEN,
						&td->peer->key,
						&enclen);
	ssize_t s=send(td->transfer->sock, data, enclen, 0);
	if(s<0)
	{
		perror("Send");
		return -1;
	}
	else
	{
		printf("Sent %lu bytes of encrypted data.\n", s); 
		return read ? FILE_PART_BYTES : (td->transfer->file_size)-(td->partnum*FILE_PART_BYTES);
	}
}

static void* send_file_part_thread(void* args)
{
	struct file_part_thread_data* td=args;
	send_file_part(td);
	pthread_mutex_unlock(&td->transfer->file_lock);
	free(td);
	return 0;
}

static void* sendmetadata(void* args)
{
	struct file_part_thread_data* td=args;
	ssize_t sent=0;
	do
	{
		ssize_t read=send_file_part(td);
		sent+=read ? FILE_PART_BYTES : td->transfer->file_size;
		printf("Sent %lu bytes of %lu\n", sent, td->transfer->file_size); 
	} while(sent < td->transfer->file_size);

	clear_file_transfer(td->transfer);
	pthread_mutex_unlock(&td->transfer->file_lock);
	free(td);
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

static FILE* check_peer_and_open_file_for_sha(struct peer* p, const char* sha_str)
{
	if(!p) return NULL;

	FILE* ret=NULL;
	struct config* conf=getconfig();
	struct configitem* ci;
	if((ci=config_find_item(conf, "Filename", sha_str)) && ci->val)
	{
		ret=fopen(ci->val, "r");
		if(!ret)
		{
			printf("Could not open file for hash %s\n", sha_str);
		}
	}
	return ret;
}

static FILE* check_peer_and_open_metadata(struct peer* p, const char* sha_str)
{
	if(!p) return NULL;

	FILE* ret=NULL;
	struct config* conf=getconfig();
	struct configitem* ci;
	if((ci=config_find_item(conf, "Filename", sha_str)) && ci->val)
	{
		char* mdpath=NULL;
		getpath(metadatapath(), sha_str, &mdpath);
		ret=fopen(mdpath, "r");
		if(!ret)
		{
			printf("Could not open metadata file %s\n", mdpath);
		}
		free(mdpath);
	}
	return ret;
}

static file_t* get_and_lock_filetransfer_for_peer(struct peer* p)
{
	for(int i=0; i<MAX_TRANSFERS; ++i)
	{
		if(pthread_mutex_trylock(&p->file_transfers[i].file_lock) == 0 &&
			p->file_transfers[i].sock == 0)
		{
			return &(p->file_transfers[i]);
		}
	}
	return NULL;
}

void handlefiletransfer(evt_t* e, void* data)
{
	const char* filehash=(const char*)e->data;
	struct peer* p=peer_from_event(e);
	FILE* f=check_peer_and_open_metadata(p, filehash);
	if(f)
	{
		file_t* t=get_and_lock_filetransfer_for_peer(p);
		if(t)
		{
			t->sock=p->osock==SOCKET_ONEWAY?p->isock:p->osock;
			t->part_count=0;
			t->file_size=0;
			t->file=f;

			pthread_t sendthread;
			struct file_part_thread_data* td=malloc(sizeof(struct file_part_thread_data));
			td->partnum=0;
			td->peer=p;
			td->transfer=t;

			pthread_create(&sendthread, NULL, &sendmetadata, td);
			pthread_mutex_unlock(&t->file_lock);
		}
		else
		{
			printf("Maximum amount of file transfers for peer %s in use\n", p->addr);
		}
	}
}

void handlefilepartrequest(evt_t* e, void* data)
{
	struct peer* p=peer_from_event(e);
	fprequest_t* req=(fprequest_t*)e->data;
	printf("File part %d requested for hash %s\n", req->part, req->sha_str);
	FILE* f=check_peer_and_open_file_for_sha(p, (const char*)req->sha_str);
	if(f)
	{
		file_t* t=get_and_lock_filetransfer_for_peer(p);
		if(t)
		{
			t->sock=p->osock==SOCKET_ONEWAY?p->isock:p->osock;
			t->part_count=0;
			t->file_size=0;
			t->file=f;

			struct file_part_thread_data* td=malloc(sizeof(struct file_part_thread_data));
			td->partnum=req->part;
			td->peer=p;
			td->transfer=t;
			pthread_t sendthread;
			pthread_create(&sendthread, NULL, &send_file_part_thread, td);
		}
		else
		{
			printf("Maximum amount of file transfers for peer %s in use\n", p->addr);
			fclose(f);
		}
	}
}
