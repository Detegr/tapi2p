#include "peermanager.h"

static int m_peersize=5;
static int m_peercount=0;
static struct peer* m_peers=NULL;

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
