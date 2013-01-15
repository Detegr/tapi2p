#ifndef TAPI2P_PEER_H
#define TAPI2P_PEER_H

#include <pthread.h>
#include "../crypt/publickey.h"

#define IPV4_MAX 16

struct peer
{
	char addr[IPV4_MAX];
	unsigned short port;

	int m_connectable;
	int isock;
	int osock;
	pthread_t thread;
	struct pubkey key;
};

void peer_init(struct peer* p);
void peer_free(struct peer* p);

#endif
