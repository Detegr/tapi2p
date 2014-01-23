#include "../../core/pathmanager.h"
#include "../../core/event.h"
#include "../../core/pipemanager.h"
#include "../../crypto/publickey.h"
#include "../../core/peermanager.h"
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
			printf(	"%s%s  %s%s  %-20s%s%s",
					"tapi2p -- Adding peers\n",
					"First argument:\n",
					"Ip address of the peer to be added\n",
					"Flags needed:\n",
					"-p|--port", "Port which peer uses\n",
					"Example: ./tapi2p --add-peer 192.168.1.2 --port 55555\n");
			break;
		}
		case usage_setup:
		{
			printf(	"%s%s  %s%s  %-20s%s%s",
					"tapi2p -- Setting up tapi2p\n",
					"First argument:\n",
					"Nickname for your own use\n",
					"Flags needed:\n",
					"-p|--port", "Port in which you accept connections from other peers\n",
					"Example: ./tapi2p --setup Tapi2pUser --port 50000\n");
			break;
		}
		case usage_help:
		{
			printf(	"%s%s  %-20s%s  %-20s%s  %-20s%s  %-20s%s",
					"tapi2p -- Usage\n",
					"Flags:\n",
					"-a|--add-peer",    "Add peer to config file. See ./tapi2p --help add-peer\n",
					"-h|--help",        "This help. For help about specific command, use: ./tapi2p --help command\n",
					"-s|--setup",       "Set up user information for tapi2p\n",
					"-d|--dump-pubkey", "Dump your public key for sending\n");
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
	char peer_key_str[PATH_MAX];
	memset(peer_key_str, 0, PATH_MAX);

	int port=check_port(peer_port_str);
	if(port==-1) return;

	if(check_ip(peer_ip_str))
	{
		fprintf(stderr, "Ip is not a valid ipv4 ip address.\n");
		usage(usage_add_peer);
		return;
	}

	stpcpy(stpcpy(peer_key_str, "public_key_"), peer_ip_str);

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

void handlelistpeers(pipeevt_t* e, void* data)
{
	for(unsigned int i=0; i<e->data_len; ++i)
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
		{"help",		required_argument, 0, 'h'},
		{"message",		required_argument, 0, 'm'},
		{"filepart",    no_argument      , 0, 'f'},
		{"list-peers",	no_argument      , 0, 'l'},
		{"dump-key",    no_argument      , 0, 'd'},
		{"import-key",  required_argument, 0, 'i'},
		{"list-files",  optional_argument, 0, 'L'},
		{0, 0, 0, 0}
	};

	char* add_peer_args[3]={0,0,0};
	char* setup_args[2]={0,0};

	pipe_event_addlistener(ListPeers, handlelistpeers, NULL);

	opterr=0;
	for(;;)
	{
		int c=getopt_long(argc, argv, "s:a:p:h:m:ldi:f:L", options, &optind);
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
				if(add_peer_args[0])
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
				pipeevt_t* e=event_recv(fd, NULL);
				free(e);
				close(fd);
				return 0;
			}
			case 'L':
			{
				int fd=core_socket();
				char *ip=NULL;
				unsigned short port=0;
				if(argc>=3)
				{
					ip=argv[2];
					port=atoi(argv[3]);
				}
				if(!ip)
				{
					if(event_send_simple(RequestFileListLocal, NULL, 0, fd) == -1)
					{
						fprintf(stderr, "Error sending an event!\n");
					}
					pipeevt_t* e=event_recv(fd, NULL);
					printf("%s\n", e->data);
					free(e);
				}
				else
				{
					if(event_send_simple_to_addr(RequestFileListLocal, NULL, 0, ip, port, fd) == -1)
					{
						fprintf(stderr, "Error sending an event!\n");
					}
					pipeevt_t* e=event_recv(fd, NULL);
					printf("%s\n", e->data);
					free(e);
				}
				close(fd);
				return 0;
			}
			case 'm':
			{
				int fd=core_socket();
				evt_t* e=event_new(Message, (const unsigned char*)optarg, strlen(optarg)+1);
				if(event_send((pipeevt_t*)e, fd) == -1)
				{
					fprintf(stderr, "Error sending an event!\n");
				}
				free(e);
				return 0;
			}
			case 'f':
			{
				if(argc==5)
				{
					int fd=core_socket();
					if(event_send_simple_to_addr(RequestFileTransferLocal, argv[4], strlen(argv[4])+1, argv[2], atoi(argv[3]), fd) == -1)
					{
						fprintf(stderr, "Error sending an event!\n");
					}
				}
				else usage(usage_help);
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
			case 'd':
			{
				static struct pubkey selfkey;
				if(pubkey_load(&selfkey, selfkeypath_pub()))
				{
					fprintf(stderr, "Failed to load private key. Have you set up tapi2p correctly?\n");
					return -1;
				}
				printf("%s", selfkey.keyastext);
				return 0;
			}
			case 'i':
			{
				if(argc > 3)
				{
					printf("Too many arguments\n");
					return 0;
				}
				if(argc==3)
				{
					struct config* conf=getconfig();
					struct configsection* peers=config_find_section(conf, "Peers");
					struct configsection* peer=NULL;

					if(!peers)
					{
						printf("No peers found\n");
						return 0;
					}
					if(!(peer=config_find_section(conf, argv[2])))
					{
						for(unsigned i=0; i<peers->itemcount; ++i)
						{
							if(peers->items[i]->key)
							{
								struct configitem* nick=config_find_item(conf, "Nick", peers->items[i]->key);
								if(nick && nick->val)
								{
									if(strncmp(nick->val, argv[2], ITEM_MAXLEN) == 0)
									{
										peer=config_find_section(conf, peers->items[i]->key);
										break;
									}
								}
							}
						}
					}
					if(!peer)
					{
						printf("No peer found: %s\n", argv[2]);
						return 0;
					}

					if(strlen(keypath()) + strlen("public_key_") + strlen(peer->name) > PATH_MAX)
					{
						fprintf(stderr, "Path for public key too long. Please check TAPI2P_ROOT environment variable and try again.\n");
						return -1;
					}

					printf("Paste public key for peer %s\n", peer->name);

					const size_t pubkey_size=800;
					char keybuf[pubkey_size+1]; // +1 for last \n
					memset(keybuf, 0, pubkey_size);
					unsigned read=0;
					while(read!=pubkey_size)
					{
						if(!fgets(keybuf+read, pubkey_size-read+1, stdin))
						{
							fprintf(stderr, "Failed to read public key\n");
							return -1;
						}
						read+=strnlen(keybuf+read, pubkey_size-read);
					}
					if(strncmp(keybuf, "-----BEGIN PUBLIC KEY-----\n", 27))
					{
						fprintf(stderr, "Error, not a valid public key\n");
						return -1;
					}
					else if(strncmp(keybuf+775, "-----END PUBLIC KEY-----\n", 25))
					{
						fprintf(stderr, "Error, not a valid public key\n");
						return -1;
					}

					printf("Public key ok, saving to %spublic_key_%s\n", keypath(), peer->name);
					char pubfpath[PATH_MAX];
					memset(pubfpath, 0, PATH_MAX);
					stpcpy(stpcpy(stpcpy(pubfpath, keypath()), "public_key_"), peer->name);
					FILE* pubf=fopen(pubfpath, "w+");
					if(fwrite(keybuf, 800, 1, pubf) != 1)
					{
						fprintf(stderr, "Writing public key to a file failed!\n");
						return -1;
					}
					fclose(pubf);
				}
				return 0;
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
