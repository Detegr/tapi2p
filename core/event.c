#include "event.h"
#include <stdlib.h>
#include <string.h>

void event_init(struct Event* evt, EventType t, const char* data)
{
	evt->m_type=t;
	if(data)
	{
		event_set(evt, data);
	} else evt->data=NULL;
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

const char* eventtype_str(struct Event* evt)
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

void event_free_s(struct Event* evt)
{
	if(evt->data) free(evt->data);
	if(evt->next) event_free(evt->next);
}

int event_send(struct Event* evt, int fd)
{
	char buf[EVENT_MAX];
	memset(buf, 0, EVENT_MAX);
	stpncpy(stpncpy(buf, eventtype_str(evt), EVENT_LEN), evt->data, strnlen(evt->data, EVENT_MAX-EVENT_LEN));
	if(send(fd, buf, strnlen(buf, EVENT_MAX), 0) < 0) return -1;
	return 0;
}
