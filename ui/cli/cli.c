#include "../../core/pathmanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <event.h>
#include <getopt.h>
#include <ctype.h>

static const int usage_add_peer=0;

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

int check_ip(char* ip)
{
	struct in_addr dummy;
	if(inet_aton(ip, &dummy) == 0) return -1;
	else return 0;
}

void add_peer(char** args)
{
	char* peer_ip_str=args[0];
	char* peer_port_str=args[1];

	//peer_ip_str=strtok(peerstr, ":");
	//peer_port_str=strtok(NULL, ":");

	int port=strtol(peer_port_str, NULL, 10);
	if(port==LONG_MIN || port==LONG_MAX || port<0 || port>65535)
	{
		fprintf(stderr, "Port argument is not a valid port.\n");
		return;
	}

	if(check_ip(peer_ip_str))
	{
		fprintf(stderr, "Ip is not a valid ipv4 ip address.\n");
		return;
	}

	struct config* c = getconfig();
	config_add(c, peer_ip_str, "Port", peer_port_str);
	FILE* conffile=fopen(configpath(), "w");
	config_flush(c, conffile);
	fclose(conffile);
	printf("Peer %s:%s added successfully!\n", peer_ip_str, peer_port_str);
}

void usage(int usage_page)
{
	switch(usage_page)
	{
		case 0:
		{
			printf("tapi2p -- Adding peers\n"
				   "Flags needed:\n"
				   "\t-p|--port => Port which peer uses\n"
				   "\t-k|--key  => Name of the keyfile that has the peer's public key\n");
			break;
		}
	}
}

int main(int argc, char** argv)
{
	int optind;
	static struct option options[] =
	{
		{"add-peer",	required_argument, 0, 'a'},
		{"port",		required_argument, 0, 'p'},
		{"key",			required_argument, 0, 'k'},
		{0, 0, 0, 0}
	};

	char* add_peer_args[3]={0,0,0};

	for(;;)
	{
		int c=getopt_long(argc, argv, "a:p:k:", options, &optind);
		if(c==-1) break;
		switch(c)
		{
			case 'a':
			{
				add_peer_args[0]=optarg;
				break;
			}
			case 'p':
			{
				add_peer_args[1]=optarg;
				if(add_peer_args[0] && add_peer_args[2])
				{
					add_peer(add_peer_args);
				}
				break;
			}
			case 'k':
			{
				add_peer_args[2]=optarg;
				if(add_peer_args[0] && add_peer_args[1])
				{
					add_peer(add_peer_args);
				}
				break;
			}
		}
	}
	for(int i=0; i<3; ++i)
	{
		if(!add_peer_args[i])
		{
			usage(usage_add_peer);
			break;
		}
		else printf("%s\n", add_peer_args[i]);
	}
	return;

	int fd=core_socket();
	struct Event e;
	event_init(&e, Message, "foobar");
	event_send(&e, fd);
	event_free_s(&e);
	close(fd);
	return 0;
}
