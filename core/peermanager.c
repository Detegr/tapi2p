#include "peermanager.h"
#include <assert.h>
#include <pthread.h>

static fd_set m_readset;
static fd_set m_writeset;
static int read_max_int=-1;
static int write_max_int=-1;
static int m_peersize=5;
static int m_peercount=0;
static struct peer* m_peers=NULL;
static pthread_mutex_t m_lock=PTHREAD_MUTEX_INITIALIZER;
static int m_initialized=0;
static int m_iterator=0;

struct peer* peer_new()
{
	pthread_mutex_lock(&m_lock);
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
	struct peer* ret = &m_peers[m_peercount++];
	pthread_mutex_unlock(&m_lock);
	return ret;
}

void peer_writeset(fd_set* set)
{
	FD_ZERO(set);
	pthread_mutex_lock(&m_lock);
	memcpy(set, &m_writeset, sizeof(fd_set));
	pthread_mutex_unlock(&m_lock);
}

void peer_readset(fd_set* set)
{
	FD_ZERO(set);
	pthread_mutex_lock(&m_lock);
	memcpy(set, &m_readset, sizeof(fd_set));
	pthread_mutex_unlock(&m_lock);
}

void peer_removefromset(struct peer* p)
{
	FD_CLR(p->osock, &m_writeset);
	FD_CLR(p->isock, &m_writeset);
	FD_CLR(p->isock, &m_readset);
}

int read_max()
{
	pthread_mutex_lock(&m_lock);
	int ret=read_max_int;
	pthread_mutex_unlock(&m_lock);
	return ret;
}

int write_max()
{
	pthread_mutex_lock(&m_lock);
	int ret=write_max_int;
	pthread_mutex_unlock(&m_lock);
	return ret;
}

int peer_addtoset(struct peer* p)
{
	if(m_initialized)
	{
		FD_ZERO(&m_writeset);
		FD_ZERO(&m_readset);
	}
	pthread_mutex_lock(&m_lock);
	assert(p->isock != p->osock);
	if(p->osock != -1)
	{
		FD_SET(p->osock, &m_writeset);
		if(p->osock > write_max_int) write_max_int=p->osock;
	}
	else if(p->isock != -1)
	{
		FD_SET(p->isock, &m_writeset);
		if(p->isock > write_max_int) write_max_int=p->isock;
	}
	else
	{
		pthread_mutex_unlock(&m_lock);
		fprintf(stderr, "Peer with no sockets!\n");
		return -1;
	}
	FD_SET(p->isock, &m_readset);
	if(p->isock > read_max_int) read_max_int=p->isock;
	pthread_mutex_unlock(&m_lock);
	return 0;
}

int peer_exists(struct peer* p)
{
	pthread_mutex_lock(&m_lock);
	for(int i=0; i<m_peercount; ++i)
	{
		if(strncmp(m_peers[i].addr, p->addr, IPV4_MAX) == 0 &&
		   m_peers[i].port == p->port &&
		   &m_peers[i] != p)
		{
			pthread_mutex_unlock(&m_lock);
			return 1;
		}
	}
	pthread_mutex_unlock(&m_lock);
	return 0;
}

struct peer* peer_next()
{
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_lock(&m_lock);
	if(m_iterator < m_peercount) return &m_peers[m_iterator++];
	else
	{
		m_iterator=0;
		pthread_mutex_unlock(&m_lock);
		return NULL;
	}
}

int peer_remove(struct peer* p)
{
	pthread_mutex_lock(&m_lock);
	for(int i=0; i<m_peercount; ++i)
	{
		if(memcmp(p, &m_peers[i], sizeof(struct peer)) == 0)
		{
			peer_free(&m_peers[i]);
			if(i+1 < m_peercount)
			{
				memmove(&m_peers[i], &m_peers[i+1], (m_peercount-i+1) * sizeof(struct peer));
			}
			if(m_iterator) m_iterator--;
			m_peercount--;
			pthread_mutex_unlock(&m_lock);
			return 0;
		}
	}
	pthread_mutex_unlock(&m_lock);
	return -1;
}

void peers_free()
{
	pthread_mutex_lock(&m_lock);
	free(m_peers);
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_destroy(&m_lock);
}
