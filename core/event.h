#ifndef TAPI2P_EVENT_H
#define TAPI2P_EVENT_H

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
	EventType type;
	char* data;
	unsigned int data_len;
	struct event* next;
} evt_t;

void eventsystem_start(void);
void eventsystem_stop(void);

void event_init(evt_t* evt, EventType t, const char* data);
evt_t* new_event_fromstr(const char* str);
char* event_tostr(evt_t* e);
int event_set(evt_t* evt, const char* data);
int event_send(evt_t* evt, int fd);
evt_t* event_recv(int fd, int* status);
void event_free(evt_t* evt);
void event_free_s(evt_t* evt); // Frees event that is allocated from stack

typedef void (*EventCallback)(void* args);
void event_addlistener(EventType t, EventCallback cb);

const char* eventtype_str(evt_t* evt);

#endif
