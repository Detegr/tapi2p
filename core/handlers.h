#ifndef TAPI2P_HANDLERS_H
#define TAPI2P_HANDLERS_H

#include "event.h"
#include "peer.h"

void handlemessage(evt_t* e, void* data);
void handlelistpeers(evt_t* e, void* data);
void handlefiletransfer(evt_t* e, void* data);
void fileparthandler(evt_t* e, void* data);

#endif
