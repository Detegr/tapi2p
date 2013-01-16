#ifndef TAPI2P_PEERMANAGER_H
#define TAPI2P_PEERMANAGER_H

#include "peer.h"

struct peer* peer_new();
void peer_writeset(fd_set* set);
void peer_readset(fd_set* set);
int peer_addtoset(struct peer* p);
void peer_removefromset(struct peer* p);
// Does not do any clever checking, just checks if the
// address and the  port of p matches to another peer.
// Also, it assumes that p is allocated with peer_new()
// and thus it doesn't check if p itself exists.
int peer_exists(struct peer* p);
int peer_remove(struct peer* p);
void peers_free();

#endif
