#include "core.h"
#include "../crypt/publickey.h"
#include "../crypt/keygen.h"
#include "../crypt/aes.h"
#include "peermanager.h"
#include "pathmanager.h"
#include "pipemanager.h"
#include "config.h"
#include "event.h"
#include "ptrlist.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <locale.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <assert.h>
#include <limits.h>

static int sock_in;
static int sock_out;

static int core_socket_fd=-1;
static int run_threads;
static struct privkey deckey;

static int core_init(void);
static int core_socket(void);

static void pipe_accept(void)
{
	struct timeval to;
	to.tv_sec=0; to.tv_usec=0;

	fd_set set;
	FD_SET(core_socket(), &set);

	int nfds=select(core_socket()+1, &set, NULL, NULL, &to);
	if(nfds>0 && FD_ISSET(core_socket(), &set))
	{
		int fd=accept(core_socket(), NULL, NULL);
		if(fd>0)
		{
#ifndef NDEBUG
			printf("Pipe connection!\n");
#endif
			pipe_add(fd);
		}
	}
}

static int core_socket(void)
{
	if(core_socket_fd == -1)
	{
		struct sockaddr_un u;
		unlink(socketpath());
		int fd=socket(AF_UNIX, SOCK_STREAM, 0);
		if(fd<=0)
		{
			fprintf(stderr, "Error creating unix socket!\n");
			return -1;
		}
		memset(&u, 0, sizeof(struct sockaddr_un));
		u.sun_family=AF_UNIX;
		strcpy(u.sun_path, socketpath());
		if(bind(fd, (struct sockaddr*)&u, sizeof(u)) == -1)
		{
			fprintf(stderr, "Failed to bind unix socket\n");
			unlink(socketpath());
			return -1;
		}
		if(listen(fd, 10) == -1)
		{
			fprintf(stderr, "Failed to listen unix socket\n");
			unlink(socketpath());
			return -1;
		}
		core_socket_fd=fd;
	}
	return core_socket_fd;
}

static int core_init(void)
{
	if(mkdir(basepath(), 0755) == -1 && errno!=EEXIST)
	{
		fprintf(stderr, "Failed to create tapi2p folder!\n");
		return -1;
	}
	if(mkdir(keypath(), 0755) == -1 && errno!=EEXIST)
	{
		fprintf(stderr, "Failed to create tapi2p key folder!\n");
		return -1;
	}
	struct config* conf=getconfig();
	if(!conf->size)
	{
		config_add(conf, "Account", "Nick", "Unedited config file");
		config_add(conf, "Account", "Port", "Port you're going to use for incoming connections, for example 50000");
		config_add(conf, "xxx.xxx.xxx.xxx", "Keyname", "Path for peer's key file");
		config_add(conf, "xxx.xxx.xxx.xxx", "Port", "The port your peer uses");
		FILE* f=fopen(configpath(), "w");
		config_flush(conf, f);
		fclose(f);

		FILE* selfkey=fopen(selfkeypath(), "r");
		FILE* selfkey_pub=fopen(selfkeypath_pub(), "r");
		if(!selfkey || !selfkey_pub)
		{
			if(selfkey) fclose(selfkey);
			if(selfkey_pub) fclose(selfkey_pub);
			printf("Selfkey not found, generating...");
			fflush(stdout);
			if(generate(selfkeypath(), T2PPRIVATEKEY|T2PPUBLICKEY) == -1)
			{
				fprintf(stderr, "Failed to create keypair!\n");
				return -1;
			}
			printf("OK!\n");
		}
		printf("Seems like this is the first time you're running Tapi2P\n"
				"Your config file has been created for you, please edit it now and run Tapi2P again.\n"
				"The config file you specified is found in %s\n", configpath());
		return 1;
	}
	FILE* selfkey=fopen(selfkeypath(), "r");
	FILE* selfkey_pub=fopen(selfkeypath_pub(), "r");
	if(!selfkey || !selfkey_pub)
	{
		if(selfkey) fclose(selfkey);
		if(selfkey_pub) fclose(selfkey_pub);
		printf("Selfkey not found, generating...");
		fflush(stdout);
		if(generate(selfkeypath(), T2PPRIVATEKEY|T2PPUBLICKEY) == -1)
		{
			fprintf(stderr, "Failed to create keypair!\n");
			return -1;
		}
		printf("OK!\n");
	}
	else
	{
		fclose(selfkey);
		fclose(selfkey_pub);
	}

	if(core_socket() == -1)
	{
		return -1;
	}

	pipe_init();

	return 0;
}

static int new_socket(const char* addr, const char* port)
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
			if(connect(fd, ai->ai_addr, ai->ai_addrlen))
			{
				fprintf(stderr, "Failed to connect to %s:%s\n", addr, port);
				if(ai) freeaddrinfo(ai);
				close(fd);
				return -1;
			}
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

void* connection_thread(void* args)
{
	struct config* conf=getconfig();
	unsigned short port;
	const char* portstr=config_find_item(conf, "Port", "Account")->val;
	port = atoi(portstr);
	if(port==0 || port>65535)
	{
		fprintf(stderr, "Invalid port number: %u\n", port);
		run_threads=0;
	}
	sock_in=new_socket(NULL, portstr);
	if(sock_in<0) return 0;

	fd_set set;
	FD_ZERO(&set);
	while(run_threads)
	{
		FD_ZERO(&set);
		FD_SET(sock_in, &set);
		struct timeval to;
		to.tv_sec=1; to.tv_usec=0;
		int nfds=select(sock_in+1, &set, NULL, NULL, &to);
		if(nfds>0 && FD_ISSET(sock_in, &set))
		{
			for(;nfds>0;nfds--)
			{
				struct sockaddr addr;
				socklen_t addrlen = sizeof(addr);
				int newconn=accept(sock_in, &addr, &addrlen);
				if(newconn == -1){fprintf(stderr, "Fatal error!\n"); exit(EXIT_FAILURE);}
				addr.sa_family=AF_INET;
				char hbuf[NI_MAXHOST];
				memset(hbuf, 0, NI_MAXHOST);
				if(getnameinfo(&addr, addrlen, hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST))
				{
					fprintf(stderr, "Failed to get name info\n");
					run_threads=0;
					return 0;
				}
#ifndef NDEBUG
				printf("Connection from %s:%u\n", hbuf, ntohs(((struct sockaddr_in*)&addr)->sin_port));
#endif
				struct configsection* sect;
				if((sect=config_find_section(conf, hbuf)))
				{
					struct peer* p;
					struct configitem* ci;
					if((ci=config_find_item(conf, "Port", hbuf)))
					{
						unsigned short port;
						port=atoi(ci->val);
						if(port>65535)
						{
#ifndef NDEBUG
							printf("Invalid port %u for peer %s\n", port, hbuf);
#endif
							continue;
						}
						p=peer_new();
						p->port=port;
						if(peer_exists(p))
						{
#ifndef NDEBUG
							printf("Peer exists\n");
#endif
							peer_remove(p);
							continue;
						}
					}
					else
					{
#ifndef NDEBUG
						printf("No port specified for peer %s\n", hbuf);
#endif
						continue;
					}
					if((ci=config_find_item(conf, "Keyname", hbuf)))
					{
						assert(p);
						p->isock=newconn;
						char keybuf[PATH_MAX];
						char* s=stpncpy(keybuf, keypath(), PATH_MAX);
						if(strlen(keypath()) + strnlen(ci->val, ITEM_MAXLEN) < PATH_MAX)
						{
							strncpy(s, ci->val, ITEM_MAXLEN);
						} else
						{
#ifndef NDEBUG
							printf("Key path too long.\n");
#endif
							peer_free(p);
							continue;
						}
						if(pubkey_load(&p->key, keybuf))
						{
#ifndef NDEBUG
							printf("Invalid or no key found from %s for peer %s\n", keybuf, hbuf);
#endif
							peer_free(p);
							continue;
						}
					}
					else
					{
#ifndef NDEBUG
						printf("No keyname specified for peer %s\n", hbuf);
#endif
						peer_free(p);
						continue;
					}
					p->osock=socket(AF_INET, SOCK_STREAM, 0);
					if(connect(p->osock, &addr, sizeof(addr)))
					{
#ifndef NDEBUG
						printf("%s not connectable.\n", hbuf);
#endif
						p->m_connectable=0;
						close(p->osock);
						p->osock=-1;
					}
					peer_addtoset(p);
				}
			}
		}
	}
	close(sock_in);
	return 0;
}

void* read_thread(void* args)
{
	fd_set rset;
	fd_set wset;

	const int BUFLEN=4096;
	char readbuf[BUFLEN];

	while(run_threads)
	{
		peer_readset(&rset);
		struct timeval to; to.tv_sec=1; to.tv_usec=0;
		int nfds=select(read_max()+1, &rset, NULL, NULL, &to);
		if(nfds>0)
		{
			ptrlist_t delptrs=list_new();
			struct peer* p;
			while((p=peer_next()))
			{
				if(FD_ISSET(p->isock, &rset))
				{
					ssize_t b=recv(p->isock, readbuf, BUFLEN, 0);
					assert(b<4096); // NYI
					if(b<=0)
					{
						peer_removefromset(p);
						list_add_node(&delptrs, p);
					}
					else
					{// Send received event to core pipe listeners
						size_t datalen;
						unsigned char* data=aes_decrypt_with_key(readbuf, b, &deckey, &datalen);
						struct Event* e=new_event_fromstr(data);
						if(e)
						{
							send_event(e);
							event_free(e);
						}
					}
				}
			}
			struct node* it=delptrs.head;
			for(;it;it=it->next)
			{
				peer_remove((struct peer*)it->item);
			}
			list_free(&delptrs);
		}
		else if(nfds<0)
		{
			perror("read");
		}
	}
}

static int connect_to_peer(struct configitem* peer, char* peer_ip_str)
{
	if(!peer) return -1;
	unsigned short port;
}

static int connect_to_peers()
{
	struct config* conf=getconfig();
	struct configsection* sect;
	if(sect=config_find_section(conf, "Peers"))
	{
		for(int i=0; i<sect->size; ++i)
		{
			connect_to_peer(config_find_item(conf, sect->name, NULL), sect->name);
		}
	}
}

int core_start(void)
{
	run_threads=1;
	if(!setlocale(LC_CTYPE, "en_US.UTF-8"))
	{
		fprintf(stderr, "Cannot set UTF-8 encoding. Please make sure that en_US.UTF-8 encoding is installed.\n");
	}
	int ci=core_init();
	if(ci<0)
	{
		fprintf(stderr, "Tapi2p core failed to initialize!\n");
		return -1;
	}
	else if(ci>0)
	{// Config file created
		return 0;
	}

	if(privkey_load(&deckey, selfkeypath()))
	{
		fprintf(stderr, "Failed to start tapi2p! Client's private key failed to load.\n");
		return -1;
	}

	if(connect_to_peers())
	{
		fprintf(stderr, "Failure when connecting to peers!\n");
		return -1;
	}

	pthread_t accept_connections;
	pthread_t read_connections;
	pthread_create(&accept_connections, NULL, &connection_thread, NULL);
	pthread_create(&read_connections, NULL, &read_thread, NULL);

	printf("Tapi2p core started.\n");
	while(run_threads)
	{
		pipe_accept();
		struct Event* e=poll_event();
		if(e)
		{
			printf("Message from pipe client: %s\n", e->data);
			event_free(e);
		}
	}

	printf("Tapi2p core shutting down...\n");
	pthread_join(accept_connections, NULL);
	pthread_join(read_connections, NULL);
	return 0;
}

void core_stop(void)
{
	run_threads=0;
}
