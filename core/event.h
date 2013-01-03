#ifndef TAPI2P_EVENT_H
#define TAPI2P_EVENT_H

#define EVENT_MAX 1024
#define EVENT_LEN 5
#define EVENT_TYPES 1
	
static const char* eventtypes[EVENT_TYPES] = { "EMSG:" };

struct Event
{
	enum Type
	{
		Message=0
	} EventType;

	enum Type m_type;
	char* data;
	struct Event* next;
};

void event_init(struct Event* evt, enum Type t);
int event_set(struct Event* evt, const char* data);
void event_free(struct Event* evt);
const char* event_str(struct Event* evt);

#endif
