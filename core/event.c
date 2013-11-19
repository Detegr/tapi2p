#include "event.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <poll.h>
#include <pthread.h>

#define CBMAX 32 // Because I'm lazy

static EventCallback* callbacks[EventCount]={0};
static void* callbackdatas[EventCount][CBMAX]={{0}};
static pthread_t event_thread;

evt_t* event_new(EventType t, const unsigned char* data, unsigned int data_len)
{
	evt_t* ret=malloc(sizeof(evt_t)+data_len);
	ret->type=t;
	ret->data_len=data_len;
	ret->data=(uint8_t*)ret + sizeof(evt_t);

	if(data) memcpy(ret->data, data, data_len);
	else ret->data=NULL;

	return ret;
}

int event_send(evt_t* evt, int fd)
{
	int b=send(fd, evt, sizeof(evt_t) + evt->data_len, 0);
	if(b<0) return -1;
	else return b;
}

int event_send_simple(EventType t, const unsigned char* data, unsigned int data_len, int fd)
{
	evt_t* evt=event_new(t, data, data_len);
	int ret=event_send(evt, fd);
	free(evt);
	return ret;
}

int event_send_simple_to_peer(EventType t, const unsigned char* data, unsigned int data_len, struct peer* p, int fd)
{
	return event_send_simple_to_addr(t, data, data_len, p->addr, p->port, fd);
}

int event_send_simple_to_addr(EventType t, const unsigned char* data, unsigned int data_len, const char* addr, unsigned short port, int fd)
{
	evt_t* evt=event_new(t, data, data_len);
	memcpy(evt->addr, addr, IPV4_MAX);
	evt->port=port;
	int ret=event_send(evt, fd);
	free(evt);
	return ret;
}

int pipe_event_send_back_to_caller(pipeevt_t* e, const unsigned char* data, unsigned int data_len)
{
	return event_send_simple(e->type, data, data_len, e->fd_from);
}

void event_run_callbacks(evt_t* e)
{
	if(callbacks[e->type])
	{
		for(int j=0; j<CBMAX; ++j)
		{
			if(callbacks[e->type][j]) callbacks[e->type][j](e, callbackdatas[e->type][j]);
		}
	}
}

void pipe_event_run_callbacks(pipeevt_t* e)
{
	event_run_callbacks((evt_t*)e);
}

pipeevt_t* event_recv(int fd, int* status)
{
	pipeevt_t* evt=malloc(sizeof(pipeevt_t));
	int b;

	if((b=recv(fd, evt, sizeof(pipeevt_t), 0)) != sizeof(pipeevt_t))
	{
		if(b==0)
		{
			if(status) *status = 1;
			return NULL;
		}
		else if(b==-1)
		{
			perror("event_recv");
			exit(1);
		}
		fprintf(stderr, "Failed to read pipe event. Got %d bytes, expected %lu\n", b, sizeof(pipeevt_t));
		if(status) *status=-1;
		return NULL;
	}
	evt=realloc(evt, sizeof(pipeevt_t) + evt->data_len);
	evt->data=(uint8_t*)evt + sizeof(pipeevt_t);
	if(evt->data_len>0)
	{
		if(recv(fd, evt->data, evt->data_len, 0) < 0)
		{
			fprintf(stderr, "Failed to read event data\n");
			if(status) *status=-2;
			return NULL;
		}
	}
	evt->fd_from=fd;

	pipe_event_run_callbacks(evt);

	if(status) *status=0;
	return evt;
}

void event_addlistener(EventType t, EventCallback cb, void* data)
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
		if(!cbs[i]) {cbs[i]=cb; callbackdatas[t][i]=data; return;}
	}
	fprintf(stderr, "Maximum number of callbacks already defined, not adding a new one!\n");
	return;
}

void pipe_event_addlistener(EventType t, PipeEventCallback cb, void* data)
{
	event_addlistener(t, (EventCallback)cb, data);
}

static int run_event_thread=1;
static void* event_threadfunc(void* args)
{
	int fd=*(int*)args;
	struct pollfd fds[1];
	fds[0].fd=fd;
	fds[0].events=POLLIN;
	while(run_event_thread)
	{
		if(((poll(fds, 1, 1)) > 0) && (fds[0].revents & POLLIN))
		{
			pipeevt_t* e=event_recv(fd, NULL);
			free(e);
		}
		fds[0].fd=fd;
		fds[0].events=POLLIN;
	}
	free(args);
	return NULL;
}

void eventsystem_start(int corefd)
{
	int* fdmem=malloc(sizeof(int));
	*fdmem=corefd;
	pthread_create(&event_thread, NULL, &event_threadfunc, fdmem);
}

void eventsystem_stop(void)
{
	run_event_thread=0;
	pthread_join(event_thread, NULL);
	for(int i=0; i<EventCount; ++i)
	{
		if(callbacks[i]) free(callbacks[i]);
	}
}
