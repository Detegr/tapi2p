#include "peer.h"

void peer_init(struct peer* p)
{
	p->m_connectable=1;
	p->isock=-1;
	p->osock=-1;
	p->thread=0;
}

void peer_free(struct peer* p)
{
	if(p->thread) pthread_join(p->thread, NULL);
}
