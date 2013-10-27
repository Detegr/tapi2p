#include "../../core/pathmanager.h"
#include "../../core/event.h"
#include "../../core/pipemanager.h"
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
#include <errno.h>

typedef enum usages
{
	usage_add_peer=0,
	usage_setup,
	usage_help
} usages;

void usage(int usage_page)
{
	switch(usage_page)
	{
		case usage_add_peer:
		{
			printf(	"tapi2p -- Adding peers\n"
					"First argument:\n"
					"\tIp address of the peer to be added\n"
					"Flags needed:\n"
					"\t-p|--port - Port which peer uses\n"
					"\t-k|--key  - Name of the keyfile that has the peer's public key\n"
					"Example: ./tapi2p --add-peer 192.168.1.2 --port 55555 --key mykey\n");
			break;
		}
		case usage_setup:
		{
			printf(	"tapi2p -- Setting up tapi2p\n"
					"First argument:\n"
					"\tNickname for your own use\n"
					"Flags needed:\n"
					"\t-p|--port - Port in which you accept connections from other peers\n"
					"Example: ./tapi2p --setup Tapi2pUser --port 50000\n");
			break;
		}
		case usage_help:
		{
			printf(	"tapi2p -- Usage\n"
					"Flags:\n"
					"\t-a|--add-peer\tAdd peer to config file. See ./tapi2p --help add-peer\n"
					"\t-h|--help\tThis help. For help about specific command, use: ./tapi2p --help command\n"
					"\t-s|--setup\tSet up user information for tapi2p\n");
			break;
		}
	}
}

int check_port(char* portstr)
{
	char* endptr;
	errno=0;
	int port=strtol(portstr, &endptr, 10);
	if(port<0 || port>65535 || (errno != 0) || (endptr == portstr))
	{
		fprintf(stderr, "Port argument is not a valid port.\n");
		return -1;
	}
	return port;
}

void setup(char** args)
{
	int port=check_port(args[1]);
	if(port==-1) return;

	struct config* c = getconfig();
	config_add(c, "Account", "Nick", args[0]);
	config_add(c, "Account", "Port", args[1]);
	FILE* conffile=fopen(configpath(), "w");
	if(!conffile)
	{
		fprintf(stderr, "Couldn't open config file: %s. Have you run tapi2p_core first?\n", configpath());
		return;
	}
	config_flush(c, conffile);
	fclose(conffile);
	printf("Account setup done!\nCurrent settings:\nNick: %s\nPort: %d\n", args[0], port);
}

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
	char* peer_key_str=args[2];

	int port=check_port(peer_port_str);
	if(port==-1) return;

	if(check_ip(peer_ip_str))
	{
		fprintf(stderr, "Ip is not a valid ipv4 ip address.\n");
		usage(usage_add_peer);
		return;
	}

	struct config* c = getconfig();
	config_add(c, "Peers", peer_ip_str, NULL);
	config_add(c, peer_ip_str, "Port", peer_port_str);
	config_add(c, peer_ip_str, "Key", peer_key_str);
	FILE* conffile=fopen(configpath(), "w");
	if(!conffile)
	{
		fprintf(stderr, "Couldn't open config file: %s. Have you run tapi2p_core first?\n", configpath());
		return;
	}
	config_flush(c, conffile);
	fclose(conffile);
	printf("Peer %s:%s added successfully!\n", peer_ip_str, peer_port_str);
}

void handlelistpeers(evt_t* e, void* data)
{
	unsigned int datalen=e->data_len;
	for(unsigned int i=0; i<datalen; ++i)
	{
		printf("%c", e->data[i]);
	}
	printf("\n");
}

int main(int argc, char** argv)
{
	int optind;
	static struct option options[] =
	{
		{"setup",		required_argument, 0, 's'},
		{"add-peer",	required_argument, 0, 'a'},
		{"port",		required_argument, 0, 'p'},
		{"key",			required_argument, 0, 'k'},
		{"help",		required_argument, 0, 'h'},
		{"message",		required_argument, 0, 'm'},
		{"list",		no_argument		 , 0, 'l'},
		{0, 0, 0, 0}
	};

	char* add_peer_args[3]={0,0,0};
	char* setup_args[2]={0,0};

	event_addlistener(ListPeers, handlelistpeers, NULL);

	opterr=0;
	for(;;)
	{
		int c=getopt_long(argc, argv, "s:a:p:k:h:m:l", options, &optind);
		if(c==-1) break;
		switch(c)
		{
			case 'a':
			{
				if(argc > 7)
				{
					printf("Too many arguments.\n");
					return 0;
				}
				add_peer_args[0]=optarg;
				break;
			}
			case 'p':
			{
				setup_args[1]=optarg;
				add_peer_args[1]=optarg;
				if(add_peer_args[0] && add_peer_args[2])
				{
					add_peer(add_peer_args);
					return 0;
				}
				if(setup_args[0])
				{
					setup(setup_args);
					return 0;
				}
				break;
			}
			case 'k':
			{
				add_peer_args[2]=optarg;
				if(add_peer_args[0] && add_peer_args[1])
				{
					add_peer(add_peer_args);
					return 0;
				}
				break;
			}
			case 'h':
			{
				if(strcmp(optarg, "a") == 0 || strcmp(optarg, "add-peer")==0)
				{
					usage(usage_add_peer);
					return 0;
				}
				else if(strcmp(optarg, "s") == 0 || strcmp(optarg, "setup")==0)
				{
					usage(usage_setup);
					return 0;
				}
				else
				{
					printf("Sorry, there's no help for %s\n", optarg);
					return 0;
				}
				return 0;
			}
			case 'l':
			{
				int fd=core_socket();
				event_send_simple(ListPeers, NULL, 0, fd);
				event_recv(fd, NULL);
				close(fd);
				return 0;
			}
			case 'm':
			{
				int fd=core_socket();
				evt_t e;
				event_init(&e, Message, (const unsigned char*)optarg, strnlen(optarg, EVENT_DATALEN));
				if(event_send(&e, fd) == -1)
				{
					fprintf(stderr, "Error sending an event!\n");
				}
				event_free_s(&e);
				return 0;
			}
			case 's':
			{
				if(argc > 5)
				{
					printf("Too many arguments.\n");
					return 0;
				}
				setup_args[0]=optarg;
				break;
			}
			case '?':
			{
				//printf("%c\n", optopt);
				switch(optopt)
				{
					case 'a':
						usage(usage_add_peer);
						return 0;
					case 's':
						usage(usage_setup);
						return 0;
					case 'p':
					case 'k':
					default:
						usage(usage_help);
						return 0;
				}
			}
		}
	}
	if(add_peer_args[0] && !(add_peer_args[1] && add_peer_args[2])) usage(usage_add_peer);
	else if(setup_args[0] && !setup_args[1]) usage(usage_setup);
	else usage(usage_help);
	return 0;
}
