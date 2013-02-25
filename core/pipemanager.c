#include "pipemanager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

static fd_set pipeset;
static int pipe_fds[MAX_PIPE_LISTENERS];
static int pipe_slot;
static int fd_max;

void pipe_init(void)
{
	FD_ZERO(&pipeset);
	memset(pipe_fds, -1, MAX_PIPE_LISTENERS*sizeof(int));
	pipe_slot=0;
}

void pipe_add(int fd)
{
	for(int i=0; i<=pipe_slot; ++i)
	{
		if(pipe_fds[i]==-1)
		{
			pipe_fds[pipe_slot++]=fd;
			FD_SET(fd, &pipeset);
			if(fd>fd_max) fd_max=fd;
			fd=-1;
			break;
		}
	}
	if(fd>-1)
	{
#ifndef NDEBUG
		fprintf(stderr, "Maximum number of pipe listeners reached!!\n");
#endif
	}
}

void pipe_remove(int fd)
{
	int secondmax=0;
	for(int i=0; i<=pipe_slot; ++i)
	{
		if(pipe_fds[i]>secondmax && pipe_fds[i]<fd_max) secondmax=i;
		if(pipe_fds[i] == fd)
		{
			pipe_fds[i]=-1;
			if(i==pipe_slot) pipe_slot--;
			if(fd==fd_max) fd_max=secondmax;
			FD_CLR(fd, &pipeset);
		}
	}
}

evt_t* poll_event(void)
{
	struct timeval to;
	to.tv_sec=1; to.tv_usec=0;

	fd_set set;
	memcpy(&set, &pipeset, sizeof(fd_set));
	int nfds=select(fd_max+1, &set, NULL, NULL, &to);

	evt_t* ret=NULL;
	if(nfds>0)
	{
		evt_t* cur=NULL;
		for(int i=0; i<=pipe_slot; ++i)
		{
			if(pipe_fds[i] != -1 && FD_ISSET(pipe_fds[i], &set))
			{
				char buf[EVENT_MAX];
				memset(buf, 0, EVENT_MAX);
				int b=recv(pipe_fds[i], buf, EVENT_MAX, 0);
				if(b==EVENT_MAX)
				{
					fprintf(stderr, "Unlikely case, NYI\n");
					assert(0);
				}
				else if(b>0)
				{
					for(int j=0; j<EVENT_TYPES; ++j)
					{
						if(strncmp(buf, eventtypes[j], EVENT_LEN) == 0)
						{
							cur = (evt_t*)malloc(sizeof(evt_t));
							event_init(cur, j, buf+EVENT_LEN);
							if(!ret) ret=cur;
							else
							{
								evt_t* e=ret;
								while(e->next) e=e->next;
								e->next=cur;
							}
						}
					}
				}
			}
		}
	}
	return ret;
}

int send_event(evt_t* e)
{
	struct timeval to;
	to.tv_sec=1; to.tv_usec=0;
	fd_set set;
	memcpy(&set, &pipeset, sizeof(fd_set));
	int nfds=select(fd_max+1, NULL, &set, NULL, &to);
	if(nfds>0)
	{
		for(int i=0; i<=pipe_slot; ++i)
		{
			if(pipe_fds[i] != -1)
			{
				char buf[EVENT_MAX];
				stpncpy(stpncpy(buf, eventtypes[e->type], EVENT_LEN), e->data, EVENT_MAX-EVENT_LEN);
				size_t len=strnlen(buf, EVENT_MAX);
				if(FD_ISSET(pipe_fds[i], &set))
				{
					int s=send(pipe_fds[i], buf, len, 0);
					if(s<0)
					{
						perror("Send");
						pipe_remove(pipe_fds[i]);
						continue;
					}
#ifndef NDEBUG
					else
					{
						printf("Sent %d bytes to pipe listener #%d\n", s, pipe_fds[i]);
					}
#endif
				}
			}
		}
	}
	return 0;
}
