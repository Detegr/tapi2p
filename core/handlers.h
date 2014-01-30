#ifndef TAPI2P_HANDLERS_H
#define TAPI2P_HANDLERS_H

#include "event.h"
#include "peer.h"

void handlemessage(evt_t* e, void* data);
void handlelistpeers(pipeevt_t* e, void* data);
void handlefiletransfer(evt_t* e, void* data);
void handlefilepartrequest(evt_t* e, void* data);
void handlefiletransferlocal(evt_t* e, void* data);
void handlemetadata(evt_t* e, void* data);
void handlefilepart(evt_t* e, void* data);
void handlerequestfilelistlocal(evt_t* e, void* data);
void handlerequestfilelist(evt_t* e, void* data);
void handleaddfile(evt_t* e, void* data);
void handlesetup(pipeevt_t *e, void *data);

#endif
