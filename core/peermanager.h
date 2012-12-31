#ifndef TAPI2P_PEERMANAGER_H
#define TAPI2P_PEERMANAGER_H

#include "peer.h"

struct peer* peer_new();
int peer_remove(struct peer* p);
void peers_free();

#endif
