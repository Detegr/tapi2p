#include "peermanager.h"
#include <assert.h>
#include <pthread.h>
#include "event.h"

static fd_set m_readset;
static fd_set m_writeset;
static int read_max_int=-1;
static int write_max_int=-1;
static int m_peersize=5;
static int m_peercount=0;
static struct peer* m_peers=NULL;
static pthread_mutex_t m_lock;
static pthread_mutex_t m_writeset_lock;
static pthread_mutex_t m_readset_lock;
static int m_iterator=0;

void peermanager_init(void)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_writeset_lock, NULL);
	pthread_mutex_init(&m_readset_lock, NULL);
	pthread_mutex_init(&m_lock, &attr);
}

struct peer* peer_new(void)
{
	pthread_mutex_lock(&m_lock);
	if(!m_peers) m_peers=calloc(1, m_peersize*sizeof(struct peer));
	if(m_peercount>=m_peersize)
	{
		m_peersize*=2;
		struct peer* newp=(struct peer*)realloc(m_peers, m_peersize*sizeof(struct peer));
		if(!newp)
		{
			free(newp);
			pthread_mutex_unlock(&m_lock);
			return NULL;
		}
		else m_peers=newp;
	}
	struct peer* ret = &m_peers[m_peercount++];
	peer_init(ret);
	pthread_mutex_unlock(&m_lock);
	return ret;
}

void peer_writeset(fd_set* set)
{
	FD_ZERO(set);
	pthread_mutex_lock(&m_writeset_lock);
	memcpy(set, &m_writeset, sizeof(fd_set));
	pthread_mutex_unlock(&m_writeset_lock);
}

void peer_readset(fd_set* set)
{
	FD_ZERO(set);
	pthread_mutex_lock(&m_readset_lock);
	memcpy(set, &m_readset, sizeof(fd_set));
	pthread_mutex_unlock(&m_readset_lock);
}

void peer_removefromset(struct peer* p)
{
	pthread_mutex_lock(&m_writeset_lock);
	FD_CLR(p->osock, &m_writeset);
	FD_CLR(p->isock, &m_writeset);
	pthread_mutex_unlock(&m_writeset_lock);
	pthread_mutex_lock(&m_readset_lock);
	FD_CLR(p->isock, &m_readset);
	pthread_mutex_unlock(&m_readset_lock);
}

int read_max(void)
{
	pthread_mutex_lock(&m_readset_lock);
	int ret=read_max_int;
	pthread_mutex_unlock(&m_readset_lock);
	return ret;
}

int write_max(void)
{
	pthread_mutex_lock(&m_writeset_lock);
	int ret=write_max_int;
	pthread_mutex_unlock(&m_writeset_lock);
	return ret;
}

int peer_updateset(struct peer* p)
{
	pthread_mutex_lock(&m_writeset_lock);
	FD_CLR(p->osock, &m_writeset);
	pthread_mutex_unlock(&m_writeset_lock);

	pthread_mutex_lock(&m_readset_lock);
	FD_CLR(p->osock, &m_readset);
	pthread_mutex_unlock(&m_readset_lock);
	return peer_addtoset(p);
}

int peer_addtoset(struct peer* p)
{
	pthread_mutex_lock(&m_lock); // Required?
	pthread_mutex_lock(&m_writeset_lock);
	pthread_mutex_lock(&m_readset_lock);
	assert(p->isock != p->osock);
	if(p->osock >= 0)
	{
		FD_SET(p->osock, &m_writeset);
		if(p->osock > write_max_int) write_max_int=p->osock;
	}
	else if(p->isock >= 0)
	{
		FD_SET(p->isock, &m_writeset);
		if(p->isock > write_max_int) write_max_int=p->isock;
	}
	else
	{
		pthread_mutex_unlock(&m_lock);
		pthread_mutex_unlock(&m_writeset_lock);
		pthread_mutex_unlock(&m_readset_lock);
		fprintf(stderr, "Peer with no sockets!\n");
		return -1;
	}
	if(p->isock >= 0)
	{
		FD_SET(p->isock, &m_readset);
		if(p->isock > read_max_int) read_max_int=p->isock;
	}
	else
	{// Oneway
		FD_SET(p->osock, &m_readset);
		if(p->osock > read_max_int) read_max_int=p->osock;
	}
	pthread_mutex_unlock(&m_writeset_lock);
	pthread_mutex_unlock(&m_readset_lock);
	pthread_mutex_unlock(&m_lock); // Required?
	return 0;
}

struct peer* peer_exists(struct peer* p)
{
	pthread_mutex_lock(&m_lock);
	for(int i=0; i<m_peercount; ++i)
	{
		if(strncmp(m_peers[i].addr, p->addr, IPV4_MAX) == 0 &&
		   m_peers[i].port == p->port &&
		   &m_peers[i] != p)
		{
			struct peer* ret=&m_peers[i];
			assert(ret);
			pthread_mutex_unlock(&m_lock);
			return ret;
		}
	}
	pthread_mutex_unlock(&m_lock);
	return NULL;
}

struct peer* peer_exists_simple(char* addr, unsigned short port)
{
	if(!addr || !port) return NULL;

	struct peer* sp=peer_new();
	strncpy(sp->addr, addr, IPV4_MAX);
	sp->port=port;
	struct peer* ret=peer_exists(sp);
	peer_remove(sp);
	return ret;
}

struct peer* peer_next()
{
	pthread_mutex_unlock(&m_lock);
	pthread_mutex_lock(&m_lock);
	if(m_iterator < m_peercount)
	{
		return &m_peers[m_iterator++];
	}
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
			if(m_iterator == m_peercount) m_iterator--;
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
	pthread_mutex_destroy(&m_writeset_lock);
	pthread_mutex_destroy(&m_readset_lock);
}

struct peer* peer_from_event(struct event* e)
{
	return peer_exists_simple(e->addr, e->port);
}
