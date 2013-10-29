#include "handlers.h"
#include "core.h"
#include "peermanager.h"
#include "../dtgconf/src/config.h"

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
		if(!(p->m_connectable && p->isock>0)) dp=stpcpy(dp, " <One-way>");
		dp=stpcpy(dp, "\n");
	}
	dp[-1]=0;
	printf("%s\n", edata);
	event_set(e, edata, strnlen(edata, EVENT_DATALEN));
	event_send(e, e->fd_from);
}

void handlefiletransfer(evt_t* e, void* data)
{
	FILE* f=fopen("testfile.mp3", "r");
	//event_init(&e, FilePart, 
}

void fileparthandler(evt_t* e, void* data)
{
}
