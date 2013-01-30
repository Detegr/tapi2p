#include "../../core/pathmanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <event.h>

int core_socket()
{
	struct sockaddr_un u;
	int fd=socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd<=0)
	{
		fprintf(stderr, "Error creating unix socket!\n");
		return -1;
	}
	memset(&u, 0, sizeof(struct sockaddr_un));
	u.sun_family=AF_UNIX;
	strcpy(u.sun_path, socketpath());
	if(connect(fd, (struct sockaddr*)&u, sizeof(u)))
	{
		perror("Connect");
		return -1;
	}

	return fd;
}

int main()
{
	int fd=core_socket();
	struct Event e;
	event_init(&e, Message, "foobar");
	event_send(&e, fd);
	event_free_s(&e);
	close(fd);
	return 0;
}
