#include "event.h"
#include <stdlib.h>
#include <string.h>

void event_init(struct Event* evt, enum Type t)
{
	evt->m_type=t;
	evt->data=NULL;
	evt->next=NULL;
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
