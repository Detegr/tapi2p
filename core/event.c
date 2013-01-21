#include "event.h"
#include <stdlib.h>
#include <string.h>

void event_init(struct Event* evt, EventType t)
{
	evt->m_type=t;
	evt->data=NULL;
	evt->next=NULL;
}

struct Event* new_event_fromstr(const char* str)
{
	for(int i=0; i<EVENT_TYPES; ++i)
	{
		if(strncmp(str, eventtypes[i], EVENT_LEN) == 0)
		{
			struct Event* ret=(struct Event*)malloc(sizeof(struct Event));
			ret->m_type=i;
			ret->data=strndup(str+EVENT_LEN, EVENT_MAX-EVENT_LEN);
			ret->next=NULL;
			return ret;
		}
	}
	return NULL;
}

const char* event_str(struct Event* evt)
{
	return eventtypes[evt->m_type];
}

int event_set(struct Event* evt, const char* data)
{
	evt->data = strndup(data, EVENT_MAX-EVENT_LEN);
	return evt->data == NULL;
}

void event_free(struct Event* evt)
{
	if(evt->data) free(evt->data);
	if(evt->next) event_free(evt->next);
	free(evt);
}
