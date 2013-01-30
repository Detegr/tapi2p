#ifndef TAPI2P_EVENT_H
#define TAPI2P_EVENT_H

#define EVENT_MAX 1024
#define EVENT_LEN 5
#define EVENT_TYPES 1
	
static const char* eventtypes[EVENT_TYPES] = { "EMSG:" };

typedef enum {
	Message=0
} EventType;

struct Event
{
	EventType m_type;
	char* data;
	struct Event* next;
};

void event_init(struct Event* evt, EventType t, const char* data);
struct Event* new_event_fromstr(const char* str);
int event_set(struct Event* evt, const char* data);
int event_send(struct Event* evt, int fd);
void event_free(struct Event* evt);
void event_free_s(struct Event* evt); // Frees event that is allocated from stack

const char* eventtype_str(struct Event* evt);

#endif
