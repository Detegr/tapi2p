#ifndef TAPI2P_EVENT_H
#define TAPI2P_EVENT_H

#define EVENT_MAX 1024
#define EVENT_LEN 5
#define EVENT_TYPES 2
	
extern const char* eventtypes[EVENT_TYPES];

typedef enum {
	Message=0,
	ListPeers
} EventType;

typedef struct event
{
	EventType type;
	char* data;
	int data_len;
	struct event* next;
} evt_t;

void event_init(evt_t* evt, EventType t, const char* data);
evt_t* new_event_fromstr(const char* str);
int event_set(evt_t* evt, const char* data);
int event_send(evt_t* evt, int fd);
evt_t* event_recv(int fd);
void event_free(evt_t* evt);
void event_free_s(evt_t* evt); // Frees event that is allocated from stack

const char* eventtype_str(evt_t* evt);

#endif
