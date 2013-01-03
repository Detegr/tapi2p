#include "pipemanager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

void pipe_init(void)
{
	FD_ZERO(&pipeset);
	memset(pipe_fds, -1, MAX_PIPE_LISTENERS*sizeof(int));
	memset(pipe_slot, 0, 2*sizeof(int));
}

void pipe_add(int fd)
{
	if(fd>fd_max) fd_max=fd;
	if(pipe_slot[1] != -1)
	{
		if(pipe_slot[0] == -1)
		{
			pipe_fds[pipe_slot[1]++]=fd;
		}
		else
		{
			pipe_fds[pipe_slot[0]]=fd;
			pipe_slot[0]=-1;
		}
	}
	else
	{
		fprintf(stderr, "Maximum number of pipe listeners reached!!\n");
	}
}

void pipe_remove(int fd)
{
	int secondmax=0;
	for(int i=0; pipe_fds[i]!=-1 && i<MAX_PIPE_LISTENERS; ++i)
	{
		if(pipe_fds[i]>secondmax && pipe_slot[i]<fd_max) secondmax=i;
		if(pipe_fds[i] == fd)
		{
			pipe_fds[i]=-1;
			pipe_slot[0]=i;
			FD_CLR(fd, &pipeset);
		}
	}
	if(fd==fd_max) fd_max=secondmax;
}

struct Event* poll_event(void)
{
	struct timeval to;
	to.tv_sec=1; to.tv_usec=0;

	fd_set set;
	memcpy(&set, &pipeset, sizeof(fd_set));
	int nfds=select(fd_max+1, &set, NULL, NULL, &to);

	struct Event* ret=NULL;
	if(nfds>0)
	{
		struct Event* cur=NULL;
		for(int i=0; (pipe_fds[i] != -1 || pipe_slot[1] != -1) && i<MAX_PIPE_LISTENERS; ++i)
		{
			if(pipe_fds[i] == -1) continue;
			if(FD_ISSET(pipe_fds[i], &set))
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
							cur = (struct Event*)malloc(sizeof(struct Event));
							cur->m_type=j;
							cur->data=strndup(buf+EVENT_LEN, EVENT_MAX-EVENT_LEN);
							cur->next=NULL;
							if(!ret) ret=cur;
							else
							{
								struct Event* e=ret;
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

