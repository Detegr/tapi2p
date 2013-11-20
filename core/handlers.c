#include "handlers.h"
#include "core.h"
#include "peermanager.h"
#include "pathmanager.h"
#include "../dtgconf/src/config.h"
#include "util.h"
#include "event.h"
#include "../crypto/aes.h"

#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>

struct file_part_thread_data
{
	EventType type;
	int8_t sha_str[SHA_DIGEST_LENGTH*2+1];
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
	size_t size=read ? FILE_PART_BYTES : (td->transfer->file_size);
	evt_t* evt;
	if(td->type == Metadata)
	{
		evt=event_new(td->type, buf, size);
	}
	else
	{
		fp_t* part=alloca(sizeof(fp_t) + size);
		printf("SHASTR: %s\n", td->sha_str);
		memcpy(part->sha_str, td->sha_str, SHA_DIGEST_LENGTH*2+1);
		part->partnum=td->partnum;
		part->data=(uint8_t*)part + sizeof(fp_t);
		memcpy(part->data, buf, size);
		evt=event_new(td->type, (uint8_t*)part, sizeof(fp_t) + size);
	}
	enc_t* data=aes_encrypt_random_pass(
						(uint8_t*)evt,
						sizeof(evt_t) + evt->data_len,
						PW_LEN,
						&td->peer->key,
						&enclen);
	ssize_t s=send(td->transfer->sock, data, enclen, 0);
	free(data);
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

void handlelistpeers(pipeevt_t* e, void* data)
{
	printf("ListPeers\n");
	struct config* c=(struct config*)data;
	char edata[1024];
	memset(edata, 0, 1024);
	char* dp=edata;
	struct peer* p;
	while((p=peer_next()))
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
	pipe_event_send_back_to_caller(e, (const unsigned char*)edata, strnlen(edata, 1023)+1);
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
		ret=fopen(ci->val, "r+");
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
	printf("File transfer requested for hash %s\n", filehash);
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
			td->type=Metadata;
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
	else
	{
		printf("Failed to open file\n");
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
			td->type=FilePart;
			td->partnum=req->part;
			td->peer=p;
			td->transfer=t;
			memset(td->sha_str, 0, SHA_DIGEST_LENGTH*2+1);
			strcpy((char*)td->sha_str, (const char*)req->sha_str);

			pthread_t sendthread;
			pthread_create(&sendthread, NULL, &send_file_part_thread, td);
		}
		else
		{
			printf("Maximum amount of file transfers for peer %s in use\n", p->addr);
			fclose(f);
		}
	}
	else
	{
		printf("Failed to open file\n");
	}
}

void handlemetadata(evt_t* e, void* data)
{
#ifndef NDEBUG
	printf("Metadata found\n");
#endif
	struct peer* p=peer_from_event(e);
	if(p)
	{
		char sha_str[2*SHA_DIGEST_LENGTH+1];
		sha_to_str(e->data, sha_str);
		check_or_create_metadata(e->data, e->data_len);
		request_file_part_from_peer(0, sha_str, p);
	}
	else
	{
		printf("Peer not found\n");
	}
}

void handlefilepart(evt_t* e, void* data)
{
	struct peer* p=peer_from_event(e);
	fp_t* fp=(fp_t*)e->data;
	fp->data=(uint8_t*)fp + sizeof(fp_t);
	printf("Received FilePart %d for %s\n", fp->partnum, fp->sha_str);
	FILE* f=check_peer_and_open_file_for_sha(p, (const char*)fp->sha_str);
	if(f)
	{
		fseek(f, FILE_PART_BYTES*fp->partnum, SEEK_SET);
		size_t b=fwrite(fp->data, e->data_len-sizeof(fp_t), 1, f);
		if(b)
		{
			printf("Wrote %lu bytes (part %d)\n", e->data_len-sizeof(fp_t), fp->partnum);
		}
		else
		{
			printf("Couldn't write to file\n");
		}
		fclose(f);
	}
}
