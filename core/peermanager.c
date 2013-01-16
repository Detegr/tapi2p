#include "peermanager.h"
#include <assert.h>

static fd_set m_readset;
static fd_set m_writeset;
static int m_peersize=5;
static int m_peercount=0;
static struct peer* m_peers=NULL;
static int m_initialized=0;

struct peer* peer_new()
{
	if(!m_peers) m_peers=(struct peer*)malloc(m_peersize*sizeof(struct peer));
	if(m_peercount>=m_peersize)
	{
		m_peersize*=2;
		struct peer* newp=(struct peer*)realloc(m_peers, m_peersize*sizeof(struct peer));
		if(!newp)
		{
			free(newp);
			return NULL;
		}
		else m_peers=newp;
	}
	peer_init(&m_peers[m_peercount]);
	return &m_peers[m_peercount++];
}

void peer_writeset(fd_set* set)
{
	FD_ZERO(set);
	memcpy(set, &m_writeset, sizeof(fd_set));
}

void peer_readset(fd_set* set)
{
	FD_ZERO(set);
	memcpy(set, &m_readset, sizeof(fd_set));
}

void peer_removefromset(struct peer* p)
{
	FD_CLR(p->osock, &m_writeset);
	FD_CLR(p->isock, &m_writeset);
	FD_CLR(p->isock, &m_readset);
}

int peer_addtoset(struct peer* p)
{
	if(!m_initialized)
	{
		FD_ZERO(&m_readset);
		FD_ZERO(&m_writeset);
		m_initialized=1;
	}
	assert(p->isock != p->osock);
	if(p->osock != -1) FD_SET(p->osock, &m_writeset);
	else if(p->isock != -1) FD_SET(p->isock, &m_writeset);
	else
	{
		fprintf(stderr, "Peer with no sockets!\n");
		return -1;
	}
	FD_SET(p->isock, &m_readset);
	return 0;
}

int peer_exists(struct peer* p)
{
	for(int i=0; i<m_peercount; ++i)
	{
		if(strncmp(m_peers[i].addr, p->addr, IPV4_MAX) == 0 &&
		   m_peers[i].port == p->port &&
		   &m_peers[i] != p)
		{
			return 1;
		}
	}
	return 0;
}

int peer_remove(struct peer* p)
{
	for(int i=0; i<m_peercount; ++i)
	{
		if(memcmp(p, &m_peers[i], sizeof(struct peer)) == 0)
		{
			peer_free(&m_peers[i]);
			if(i+1 < m_peercount)
			{
				memmove(&m_peers[i], &m_peers[i+1], (m_peercount-i+1) * sizeof(struct peer));
			}
			m_peercount--;
			return 0;
		}
	}
	return -1;
}

void peers_free()
{
	free(m_peers);
}
