#include "../core/peermanager.h"
#include <assert.h>

#define PEERS 1024
#define PEERH (PEERS>>1)

struct peer* new_peer(int i)
{
	struct peer* p=peer_new();
	if(!p) exit(EXIT_FAILURE);
	p->isock=4+i;
	return p;
}

struct peer* new_connected_peer(int i)
{
	struct peer* p=peer_new();
	if(!p) exit(EXIT_FAILURE);
	p->isock=i;
	p->osock=(i*4);
	return p;
}

void create_peers(struct peer** to, struct peer** to2)
{
	for(int i=0; i<PEERH; i++)
	{
		struct peer* p1=new_peer(i);
		struct peer* p2=new_connected_peer(PEERH+i+4);
		to[i]=p1;
		to2[i]=p2;
	}
}

int main()
{
	struct peer* peers[PEERH];
	struct peer* connected_peers[PEERH];
	memset(peers, 0, PEERH);
	memset(connected_peers, 0, PEERH);
	int used[PEERH];
	memset(used, -1, PEERS*sizeof(int));

	create_peers(peers, connected_peers);
	printf("Removing peers in ascending order...\n");
	for(int i=0; i<PEERH; ++i)
	{
		peer_addtoset(peers[i]);
		peer_addtoset(connected_peers[i]);
	}
	for(int i=0; i<PEERH; ++i)
	{
		peer_removefromset(peers[i]);
		peer_removefromset(connected_peers[i]);
		peer_remove(peers[i]);
		peer_remove(connected_peers[i]);
	}

	create_peers(peers, connected_peers);
	printf("Removing peers in descending order...\n");
	for(int i=(PEERS>>1)-1; i>=0; --i)
	{
		peer_remove(peers[i]);
		peer_remove(connected_peers[i]);
	}

	create_peers(peers, connected_peers);
	printf("Removing peers in random order...\n");
	for(int i=0; i<PEERH; ++i)
	{
		int u=0;
		int r=rand()%PEERS;
		for(unsigned int j=0; j<PEERS; ++j)
		{
			if(used[j]==r)
			{
				i--;
				u=1;
				break;
			}
		}
		if(u==1) continue;
		else used[i]=r;
	}
	for(int i=0; i<PEERH; ++i)
	{
		peer_remove(peers[i]);
		peer_remove(connected_peers[i]);
	}

	printf("Testing add/remove after each other...\n");
	create_peers(peers, connected_peers);
	for(int i=0; i<PEERH>>1; ++i)
	{
		peer_remove(peers[i]);
		peers[i]=new_peer(i);
		peer_remove(connected_peers[i]);
		connected_peers[i]=new_connected_peer(i);
	}
	for(int i=0; i<PEERH; ++i)
	{
		peer_remove(peers[i]);
		peer_remove(connected_peers[i]);
	}
	peers_free();
}
