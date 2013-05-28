#ifndef TAPI2P_EVENT_H
#define TAPI2P_EVENT_H

#include <stdlib.h>
#include "peer.h"

#define EVENT_MAX 1024
#define EVENT_HEADER 5
#define EVENT_DATALEN (EVENT_MAX-EVENT_HEADER)
#define EVENT_TYPES 2
	
typedef enum {
	Message=0,
	ListPeers,
	EventCount // For iterating through eventtypes
} EventType;

typedef struct event
{
	int fd_from;
	EventType type;
	char* data; // TODO: Maybe change this to array of EVENT_MAX length.
	unsigned int data_len;
	char addr[IPV4_MAX];
	struct event* next;
} evt_t;

void eventsystem_start(int corefd);
void eventsystem_stop(void);

void event_init(evt_t* evt, EventType t, const char* data);
evt_t* new_event_fromstr(const char* str, struct peer* p);
char* event_tostr(evt_t* e);
int event_set(evt_t* evt, const char* data);
int event_send(evt_t* evt, int fd);
int event_send_simple(EventType t, const unsigned char* data, unsigned int data_len, int fd);
evt_t* event_recv(int fd, int* status);
void event_free(evt_t* evt);
void event_free_s(evt_t* evt); // Frees event that is allocated from stack

typedef void (*EventCallback)(evt_t* e, void* data);
void event_addlistener(EventType t, EventCallback cb, void* data);

const char* eventtype_str(evt_t* evt);

#endif
