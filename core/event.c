#include "event.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>

#define CBMAX 32 // Because I'm lazy

static EventCallback* callbacks[EventCount]={0};

void event_init(evt_t* evt, EventType t, const char* data)
{
	evt->type=t;
	evt->data_len=0;
	if(data)
	{
		event_set(evt, data);
	} else evt->data=NULL;
	evt->next=NULL;
}

evt_t* new_event_fromstr(const char* str)
{
	for(unsigned int i=0; i<EventCount; ++i)
	{
		if(*(unsigned char*)&str[0] == i)
		{
			evt_t* ret=(evt_t*)malloc(sizeof(evt_t));
			ret->data_len=*(unsigned int*)(str+1);
			ret->data=calloc(ret->data_len, 1);
			ret->type=i;
			memcpy(ret->data, str+EVENT_HEADER, ret->data_len);
			ret->next=NULL;
			return ret;
		}
	}
	return NULL;
}

char* event_tostr(evt_t* e)
{
	char* str=malloc(EVENT_MAX * sizeof(char));
	memcpy(str, &e->type, sizeof(unsigned char));
	memcpy(str+1, &e->data_len, sizeof(unsigned int));
	strncpy(str+EVENT_HEADER, e->data, EVENT_DATALEN);
	return str;
}

int event_set(evt_t* evt, const char* data)
{
	evt->data = strndup(data, EVENT_DATALEN);
	evt->data_len = strnlen(evt->data, EVENT_DATALEN);
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
	evt->data_len=0;
}

int event_send(evt_t* evt, int fd)
{
	char buf[EVENT_MAX];
	memset(buf, 0, EVENT_MAX);
	memcpy(buf, &evt->type, sizeof(unsigned char));
	if(evt->data)
	{
		memcpy(buf+1, &evt->data_len, sizeof(unsigned int));
		strncpy(buf+EVENT_HEADER, evt->data, evt->data_len);
	}
	if(send(fd, buf, EVENT_HEADER+evt->data_len, 0) < 0) return -1;
	return 0;
}

int event_send_simple(EventType t, const unsigned char* data, unsigned int data_len, int fd)
{
	char buf[EVENT_MAX];
	memset(buf, 0, EVENT_MAX);
	memcpy(buf, &t, sizeof(unsigned char));
	if(data)
	{
		memcpy(buf+1, &data_len, sizeof(unsigned int));
		memcpy(buf+EVENT_HEADER, data, data_len);
	}
	if(send(fd, buf, EVENT_HEADER+data_len, 0) < 0) return -1;
	return 0;
}

evt_t* event_recv(int fd, int* status)
{
	char buf[EVENT_MAX];
	int b;

	if((b=recv(fd, buf, EVENT_HEADER, 0)) != EVENT_HEADER)
	{
		if(b==0)
		{
			if(status) *status = 1;
			return NULL;
		}
		fprintf(stderr, "Failed to read event type\n");
		if(status) *status=-1;
		return NULL;
	}
	unsigned int datasize=*(unsigned int*)&buf[1];
	if(datasize>0)
	{
		if(recv(fd, buf+EVENT_HEADER, datasize, 0) < 0)
		{
			fprintf(stderr, "Failed to read event\n");
			if(status) *status=-2;
			return NULL;
		}
	}

	evt_t* e=new_event_fromstr(buf);
	e->fd_from=fd;

	if(callbacks[e->type])
	{
		for(int j=0; j<CBMAX; ++j)
		{
			if(callbacks[e->type][j]) callbacks[e->type][j](e);
		}
	}

	if(status) *status=0;
	return e;
}

void event_addlistener(EventType t, EventCallback cb)
{
	EventCallback* cbs=callbacks[t];
	if(!cbs)
	{
		cbs=malloc(CBMAX*sizeof(EventCallback*));
		memset(cbs, 0, CBMAX*sizeof(EventCallback*));
		callbacks[t]=cbs;
	}
	for(int i=0; i<CBMAX; ++i)
	{
		if(!cbs[i]) {cbs[i]=cb; return;}
	}
	fprintf(stderr, "Maximum number of callbacks already defined, not adding a new one!\n");
	return;
}

void eventsystem_start(void)
{
}

void eventsystem_stop(void)
{
	for(int i=0; i<EventCount; ++i)
	{
		if(callbacks[i]) free(callbacks[i]);
	}
}
