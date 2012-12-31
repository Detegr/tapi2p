#include "../core/peermanager.h"

#define PEERS 1024

struct peer* new_peer(int i)
{
	struct peer* p=peer_new();
	p->isock=i;
	p->osock=i*2;
	return p;
}

void create_peers(struct peer** to)
{
	for(int i=0; i<PEERS; ++i)
	{
		to[i]=new_peer(i);
	}
}

int main()
{
	struct peer* peers[PEERS];
	int used[PEERS];
	memset(used, -1, PEERS*sizeof(int));

	create_peers(peers);
	printf("Removing peers in ascending order...\n");
	for(int i=0; i<PEERS; ++i)
	{
		peer_remove(peers[i]);
	}

	create_peers(peers);
	printf("Removing peers in descending order...\n");
	for(int i=PEERS-1; i>=0; --i)
	{
		peer_remove(peers[i]);
	}

	create_peers(peers);
	printf("Removing peers in random order...\n");
	for(int i=0; i<PEERS; ++i)
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
		peer_remove(peers[r]);
	}

	printf("Testing add/remove after each other...\n");
	create_peers(peers);
	for(int i=0; i<PEERS/2; ++i)
	{
		peer_remove(peers[i]);
		peers[i]=new_peer(i);
	}
	for(int i=0; i<PEERS; ++i) peer_remove(peers[i]);
	peers_free();
}
