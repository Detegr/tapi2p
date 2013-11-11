#include "handlers.h"
#include "core.h"
#include "peermanager.h"
#include "pathmanager.h"
#include "../dtgconf/src/config.h"
#include "util.h"

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
				unsigned char filebuf[FILE_PART_BYTES];
				unsigned char buf[EVENT_MAX];
				for(int i=0; i<64; ++i)
				{
					if(p->file_transfers[i].sock == 0)
					{
						char portstr[5];
						memset(portstr, 0, 5);
						snprintf(portstr, 5, "%u", p->port);
						int filesock=new_socket(p->addr, portstr);
						if(filesock==-1)
						{
							fprintf(stderr, "Could not create socket for file transfer\n");
						}
						else
						{
							p->file_transfers[i].sock=filesock;
							p->file_transfers[i].part_count=0;
							p->file_transfers[i].file_size=0;
						}
						break;
					}
				}
				fclose(f);
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
}
