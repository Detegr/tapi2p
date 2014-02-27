#ifndef TAPI2P_EVENT_H
#define TAPI2P_EVENT_H

#include <stdlib.h>
#include "peer.h"

typedef enum {
	Message=0,
	ListPeers,
	PeerConnected,
	PeerDisconnected,
	RequestFileTransfer,
	RequestFileTransferLocal,
	RequestFilePart,
	FilePart,
	Metadata,
	RequestFileListLocal,
	RequestFileList,
	FileList,
	AddFile,
	Setup,
	Status,
	RequestFilePartList,
	FilePartList,
	FileTransferStatus,
	EventCount // For iterating through eventtypes
} EventType;

typedef struct pipe_event
{
	uint8_t  type;
	int32_t  fd_from;
	int8_t   addr[IPV4_MAX];
	uint16_t port;
	uint32_t data_len;
	uint8_t* data;
} pipeevt_t;

typedef struct event
{
	uint8_t  type;
	int32_t  dummy;
	int8_t   addr[IPV4_MAX];
	uint16_t port;
	uint32_t data_len;
	uint8_t* data;
} evt_t;

void eventsystem_start(int corefd);
void eventsystem_stop(void);

evt_t* event_new(EventType t, const void *data, unsigned int data_len);
int event_send(pipeevt_t* evt, int fd);
int event_send_simple(EventType t, const void *data, unsigned int data_len, int fd);
int event_send_simple_to_peer(EventType t, const void *data, unsigned int data_len, struct peer* p, int fd);
int event_send_simple_to_addr(EventType t, const void *data, unsigned int data_len, const char* addr, unsigned short port, int fd);
int pipe_event_send_back_to_caller(pipeevt_t* e, const void *data, unsigned int data_len);
pipeevt_t* event_recv(int fd, int* status);

typedef void (*EventCallback)(evt_t* e, void* data);
typedef void (*PipeEventCallback)(pipeevt_t* e, void* data);
void event_run_callbacks(evt_t* e);
void event_addlistener(EventType t, EventCallback cb, void* data);
void event_removelistener(EventType t, EventCallback cb);
void pipe_event_addlistener(EventType t, PipeEventCallback cb, void* data);
void pipe_event_removelistener(EventType t, PipeEventCallback cb);

const char* eventtype_str(evt_t* evt);

#endif
