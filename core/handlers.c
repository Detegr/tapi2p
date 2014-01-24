#include "handlers.h"
#include "core.h"
#include "peermanager.h"
#include "pathmanager.h"
#include "../dtgconf/src/config.h"
#include "util.h"
#include "event.h"
#include "../crypto/aes.h"
#include "peer.h"

#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <jansson.h>

struct file_part_thread_data
{
	EventType type;
	struct peer* peer;
	file_t* transfer;

	uint64_t file_size;
	uint32_t part_count;
	uint32_t partnum;
};

static ssize_t send_file_part(struct file_part_thread_data* td)
{
	printf("Sending file part\n===========\ntransfer->metadata.file_size: %llu\ntransfer->metadata.part_count %d\n", td->transfer->metadata.file_size, td->transfer->metadata.part_count);
	unsigned char buf[FILE_PART_BYTES];
	size_t read=fread(buf+(td->partnum*FILE_PART_BYTES), FILE_PART_BYTES, 1, td->transfer->file);
	size_t enclen=0;

	size_t size=read ? FILE_PART_BYTES : td->transfer->metadata.file_size;
	evt_t* evt;
	if(td->type == Metadata)
	{
		printf("Sending metadata\n");
		metadata_t* md=alloca(sizeof(metadata_t) + size);
		md->file_size=td->file_size;
		printf("Actual file size: %lu\n", md->file_size);
		md->part_count=td->part_count;
		md->data=(uint8_t*)md + sizeof(metadata_t);
		memcpy(md->data, buf, size);
		evt=event_new(td->type, (uint8_t*)md, sizeof(metadata_t) + size);
	}
	else
	{
		printf("Constructing filepart with data size of %lu\n", size);
		fp_t* part=alloca(sizeof(fp_t) + size);
		memcpy(part->sha_str, td->transfer->sha_str, SHA_DIGEST_LENGTH*2+1);
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
	int sock=td->peer->osock==SOCKET_ONEWAY?td->peer->isock:td->peer->osock;
	printf("Sending %lu bytes of filepart data to %d\n", enclen, sock);
	ssize_t s=send(sock, data, enclen, 0);
	free(data);
	free(evt);
	if(s<0)
	{
		perror("Send");
		return -1;
	}
	else
	{
		if(read) return FILE_PART_BYTES;
		else
		{
			int64_t ret=(td->transfer->metadata.file_size)-(td->transfer->metadata.part_count*FILE_PART_BYTES);
			if(ret<0) return td->transfer->metadata.file_size;
			else return (ssize_t)ret;
		}
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

static void calculate_sizes_to_metadata(FILE* f, metadata_t* md)
{
	fseek(f, 0, SEEK_END);
	md->file_size=ftell(f);
	fseek(f, 0, SEEK_SET);
	md->part_count=(md->file_size/FILE_PART_BYTES)+1;
}

static void* sendmetadata(void* args)
{
	struct file_part_thread_data* td=args;
	ssize_t sent=0;

	// Calculate and set the size of metadata file
	calculate_sizes_to_metadata(td->transfer->file, &td->transfer->metadata);
	do
	{
		sent += send_file_part(td);
		printf("Sent %lu bytes of %lu\n", sent, td->transfer->metadata.file_size); 
	} while(sent < td->transfer->metadata.file_size);

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
	struct peer* p;
	json_t *root=json_object();
	json_t *peers=json_array();
	while((p=peer_next()))
	{
		struct configitem* ci=config_find_item(c, "Nick", p->addr);
		json_t *peerobj=json_object();
		if(ci && ci->val)
		{
			json_object_set_new(peerobj, "nick", json_string(ci->val));
		}
		json_object_set_new(peerobj, "addr", json_string(p->addr));
		if(!p->m_key_ok)
		{
			json_object_set_new(peerobj, "conn_status", json_string("invalid"));
		}
		else if(!(p->m_connectable && p->isock>0))
		{
			json_object_set_new(peerobj, "conn_status", json_string("oneway"));
		}
		else
		{
			json_object_set_new(peerobj, "conn_status", json_string("ok"));
		}
		json_array_append_new(peers, peerobj);
	}
	json_object_set_new(root, "peers", peers);
	char *jsonstr=json_dumps(root, 0);
	printf("%s\n", jsonstr);
	pipe_event_send_back_to_caller(e, (const unsigned char*)jsonstr, strlen(jsonstr));
	free(jsonstr);
	json_decref(root);
}

void handlefiletransferlocal(evt_t* e, void* data)
{
	struct peer* p;
	if((p=peer_exists_simple(e->addr, e->port)))
	{
		e->type=RequestFileTransfer;
		send_event_to_peer(p,e);
		printf("Sent HandleFileTransfer to: %s:%u\n", e->addr, e->port);
	}
	else
	{
		printf("Peer %s:%u not found.\n", e->addr, e->port);
	}
}

static FILE* check_peer_and_open_file_for_sha(struct peer* p, const char* sha_str, uint64_t* filesize)
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
		if(filesize)
		{
			fseek(ret, 0, SEEK_END);
			*filesize=ftell(ret);
			fseek(ret, 0, SEEK_SET);
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

void handlefiletransfer(evt_t* e, void* data)
{
	const char* filehash=(const char*)e->data;
	printf("File transfer requested for hash %s\n", filehash);
	struct peer* p=peer_from_event(e);
	FILE* f=check_peer_and_open_metadata(p, filehash);
	if(f)
	{
		file_t* t=get_and_lock_new_filetransfer(p);
		if(t)
		{
			pthread_t sendthread;
			struct file_part_thread_data* td=malloc(sizeof(struct file_part_thread_data));
			td->type=Metadata;
			td->partnum=0;
			td->peer=p;
			td->transfer=t;

			t->file=f;

			uint64_t filesize;

			// Set file size of the transfer to the size of actual file (not metadata)
			fclose(check_peer_and_open_file_for_sha(p, filehash, &filesize));
			td->part_count=(filesize/FILE_PART_BYTES)+1;
			td->file_size=filesize;

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
	FILE* f=check_peer_and_open_file_for_sha(p, (const char*)req->sha_str, NULL);
	if(f)
	{
		file_t* t=get_and_lock_new_filetransfer(p);
		if(t)
		{
			t->file=f;
			// Calculate and set the size of the actual file
			calculate_sizes_to_metadata(t->file, &t->metadata);

			struct file_part_thread_data* td=malloc(sizeof(struct file_part_thread_data));
			td->type=FilePart;
			td->partnum=req->part;
			td->peer=p;
			td->transfer=t;
			memset(t->sha_str, 0, SHA_DIGEST_LENGTH*2+1);
			strcpy((char*)t->sha_str, (const char*)req->sha_str);

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
		metadata_t* md=(metadata_t*)e->data;
		md->data = (uint8_t*)e->data + sizeof(metadata_t);
		sha_to_str(md->data, sha_str);
		check_or_create_metadata(md->data, e->data_len-sizeof(metadata_t));
		create_file_transfer(p, sha_str, md);
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
	file_t* ft=get_and_lock_existing_filetransfer_for_sha(p, (const char*)fp->sha_str);
	if(ft)
	{
		fseek(ft->file, FILE_PART_BYTES*fp->partnum, SEEK_SET);
		size_t b=fwrite(fp->data, e->data_len-sizeof(fp_t), 1, ft->file);
		if(b>0)
		{
			fflush(ft->file);
			printf("Wrote %lu bytes (part %d)\n", e->data_len-sizeof(fp_t), fp->partnum);
			if(fp->partnum==0 && e->data_len-sizeof(fp_t) != FILE_PART_BYTES)
			{
				printf("File transfer done, clearing...\n");
				clear_file_transfer(ft);
			}
		}
		else
		{
			perror("Couldn't write to file");
		}
		pthread_mutex_unlock(&ft->file_lock);
	}
	else
	{
		printf("Could not find ongoing file transfer for %s\n", fp->sha_str);
	}
}

static json_t *get_file_list_as_json(void)
{
	struct config* conf=getconfig();
	struct configsection *cs;
	json_t *root=json_object();
	json_t *files=json_array();
	if((cs=config_find_section(conf, "Metadata")))
	{
		for(int i=0; i<cs->itemcount; ++i)
		{
			struct configitem *ci=cs->items[i];
			json_t *fileobj=json_object();
			json_object_set_new(fileobj, "filename", json_string(ci->key));
			json_object_set_new(fileobj, "hash", json_string(ci->val));
			json_array_append_new(files, fileobj);
		}
	}
	json_object_set_new(root, "files", files);
	return root;
}

void handlerequestfilelistlocal(evt_t* e, void* data)
{
	printf("Requesting file list\n");
	struct peer *p;
	if((p=peer_exists_simple(e->addr, e->port)))
	{
		e->type=RequestFileList;
		send_event_to_peer(p,e);
		printf("Sent ListFiles to: %s:%u\n", e->addr, e->port);
		return;
	}
	if(e->addr && e->port)
	{
		fprintf(stderr, "Couldn't send FileList request. Peer does not exist.\n");
		return;
	}
	else
	{
		json_t *root=get_file_list_as_json();
		char *str=json_dumps(root, 0);
		printf("%s - %d\n", str, strlen(str));
		pipe_event_send_back_to_caller((pipeevt_t*)e, (const unsigned char*)str, strlen(str));
		free(str);
		json_decref(root);
	}
}

void handlerequestfilelist(evt_t* e, void* data)
{
	printf("File list requested\n");
	json_t *root=get_file_list_as_json();
	char *str=json_dumps(root, 0);
	struct peer *p=peer_exists_simple(e->addr, e->port);
	if(p)
	{
		printf("Peer exists\n");
		evt_t *evt = event_new(FileList, str, strlen(str));
		send_event_to_peer(p, evt);
		free(evt);
		printf("Sent data to peer\n");
	}
	else
	{
		fprintf(stderr, "Failed to send EventList to peer. No such peer.\n");
	}
	free(str);
	json_decref(root);
}
