#ifndef TAPI2P_PEERMANAGER_H
#define TAPI2P_PEERMANAGER_H

#include "peer.h"
#include <stdint.h>

struct event;

/* These functions are thread-safe */

void peermanager_init(void);

struct peer* peer_new(void);
void peer_writeset(fd_set* set);
void peer_readset(fd_set* set);
int peer_addtoset(struct peer* p);
int peer_updateset(struct peer* p);
void peer_removefromset(struct peer* p);
// Does not do any clever checking, just checks if the
// address and the port of p matches to another peer.
// Also, it assumes that p is allocated with peer_new()
// and thus it doesn't check if p itself exists.
// Returns the existing peer if peer exists, NULL otherwise
struct peer* peer_exists(struct peer* p);
struct peer* peer_exists_simple(const char* addr, unsigned short port);
int peer_remove(struct peer* p);
void peers_free(void);
struct peer* peer_from_event(struct event* e);

int read_max(void);
int write_max(void);

// Used to iterate through all peers.
// Will handle locking automatically
// and release the lock when done.
struct peer* peer_next(void);

#endif
