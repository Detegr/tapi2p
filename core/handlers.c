#include "handlers.h"
#include "core.h"
#include "peermanager.h"
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

void handlefiletransfer(evt_t* e, void* data)
{
	struct peer* p;
	struct peer* sp=peer_new();
	strncpy(sp->addr, e->addr, IPV4_MAX);
	sp->port=e->port;
	printf("Addr & port: %s:%u\n", e->addr, e->port);
	if((p=peer_exists(sp)))
	{
		peer_remove(sp);
		FILE* f=fopen("testfile.mp3", "r");
		unsigned char filebuf[FILE_PART_BYTES];
		unsigned char buf[EVENT_MAX];
		for(int i=0; i<63; ++i)
		{
			if(p->file_sockets[i] == 0)
			{
				char portstr[5];
				snprintf(portstr, 5, "%u", p->port);
				int filesock=new_socket(p->addr, portstr);
				printf("Created new socket: %d for peer %s:%u\n", p->addr, p->port);
				p->file_sockets[i]=filesock;
			}
		}
	}
	else
	{
		fprintf(stderr, "HandleFileTransfer: Peer not found\n");
	}
}

void fileparthandler(evt_t* e, void* data)
{
}
