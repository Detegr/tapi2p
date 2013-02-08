#include "event.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>

void event_init(evt_t* evt, EventType t, const char* data)
{
	evt->type=t;
	if(data)
	{
		event_set(evt, data);
	} else evt->data=NULL;
	evt->next=NULL;
}

evt_t* new_event_fromstr(const char* str)
{
	for(int i=0; i<EVENT_TYPES; ++i)
	{
		if(strncmp(str, eventtypes[i], EVENT_LEN) == 0)
		{
			evt_t* ret=(evt_t*)malloc(sizeof(evt_t));
			ret->type=i;
			ret->data=strndup(str+EVENT_LEN, EVENT_MAX-EVENT_LEN);
			ret->next=NULL;
			return ret;
		}
	}
	return NULL;
}

const char* eventtype_str(evt_t* evt)
{
	return eventtypes[evt->type];
}

int event_set(evt_t* evt, const char* data)
{
	evt->data = strndup(data, EVENT_MAX-EVENT_LEN);
	return evt->data == NULL;
}

void event_free(evt_t* evt)
{
	if(evt->data) free(evt->data);
	if(evt->next) event_free(evt->next);
	free(evt);
}

void event_free_s(evt_t* evt)
{
	if(evt->data) free(evt->data);
	if(evt->next) event_free(evt->next);
}

int event_send(evt_t* evt, int fd)
{
	char buf[EVENT_MAX];
	memset(buf, 0, EVENT_MAX);
	char* p=stpncpy(buf, eventtype_str(evt), EVENT_LEN);
	if(evt->data)
	{
		stpncpy(p, evt->data, strnlen(evt->data, EVENT_MAX-EVENT_LEN));
	}
	if(send(fd, buf, strnlen(buf, EVENT_MAX), 0) < 0) return -1;
	return 0;
}

evt_t* event_recv(int fd)
{
	char buf[EVENT_MAX];

	if(recv(fd, buf, EVENT_LEN, 0) != EVENT_LEN)
	{
		fprintf(stderr, "Failed to read event type\n");
		return NULL;
	}
	if(recv(fd, buf+EVENT_LEN, EVENT_MAX-EVENT_LEN, 0) < 0)
	{
		fprintf(stderr, "Failed to read event\n");
		return NULL;
	}
	return new_event_fromstr(buf);
}
