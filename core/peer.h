#ifndef TAPI2P_PEER_H
#define TAPI2P_PEER_H

#include <pthread.h>
#include "../crypto/publickey.h"
#include "file.h"

#define IPV4_MAX 16
#define MAX_TRANSFERS 64

struct peer
{
	char addr[IPV4_MAX];
	unsigned short port;

	int m_connectable;
	int m_key_ok;
	int isock;
	int osock;
	pthread_t thread;
	struct pubkey key;

	file_t file_transfers[MAX_TRANSFERS];
};

void peer_init(struct peer* p);
void peer_free(struct peer* p);

int create_file_transfer(struct peer* p, const char* sha_str, metadata_t* metadata);
file_t *get_and_lock_new_filetransfer(struct peer* p);
file_t *get_and_lock_existing_filetransfer_for_sha(struct peer *p, const char *sha_str);
const file_t *get_file_transfer(struct peer *p, const char *sha_str);
void clear_file_transfer(file_t* transfer);

#endif
