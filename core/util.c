#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int new_socket(const char* addr, const char* port)
{
	struct addrinfo* ai=NULL;
	if(addr && port)
	{
		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family=AF_UNSPEC;
		hints.ai_socktype=SOCK_STREAM;
		if(getaddrinfo(addr, port, &hints, &ai))
		{
			fprintf(stderr, "Failed to get address info!\n");
			return -1;
		}
	}

	int fd=socket(AF_INET, SOCK_STREAM, 0);

	if(fd<0)
	{
		fprintf(stderr, "Failed to create socket!\n");
		return fd;
	}

	if(port)
	{
		if(addr)
		{
			fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
			if(connect(fd, ai->ai_addr, ai->ai_addrlen))
			{
				if(errno!=EINPROGRESS)
				{
					fprintf(stderr, "Failed to connect to %s:%s\n", addr, port);
					if(ai) freeaddrinfo(ai);
					close(fd);
					return -1;
				}
			}
			if(!check_writability(fd))
			{
				fprintf(stderr, "Socket %d connected, but not writable\n");
				if(ai) freeaddrinfo(ai);
				close(fd);
				return -1;
			}
			fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
		}
		else
		{
			int yes=1;
			setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(yes));

			struct sockaddr_in sa;
			sa.sin_family=AF_INET;
			sa.sin_port=htons(atoi(port));
			sa.sin_addr.s_addr=htons(INADDR_ANY);

			if(bind(fd, (struct sockaddr*)&sa, sizeof(sa)))
			{
				fprintf(stderr, "Failed to bind socket!\n");
				if(ai) freeaddrinfo(ai);
				close(fd);
				return -1;
			}
			if(listen(fd, 10))
			{
				fprintf(stderr, "Failed to listen to socket!\n");
				if(ai) freeaddrinfo(ai);
				close(fd);
				return -1;
			}
		}
	}
	else
	{
		fprintf(stderr, "Couldn't create socket without a port!\n");
		close(fd);
		return -1;
	}
	if(ai) freeaddrinfo(ai);
	return fd;
}

int check_writability(int fd)
{
	fd_set ws;
	FD_ZERO(&ws);
	FD_SET(fd, &ws);

	struct timeval to;
	to.tv_sec=2; to.tv_usec=0;

	return (select(fd+1, NULL, &ws, NULL, &to) > 0);
}

