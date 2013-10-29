#include "core.h"
#include "../crypto/publickey.h"
#include "../crypto/keygen.h"
#include "../crypto/aes.h"
#include "peermanager.h"
#include "pathmanager.h"
#include "pipemanager.h"
#include "../dtgconf/src/config.h"
#include "event.h"
#include "ptrlist.h"
#include "handlers.h"
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
#include <signal.h>
#include <stdlib.h>
#include <semaphore.h>

#define SOCKET_ONEWAY -2
// Length of the password used to AES encrypt data
#define PW_LEN 80

static volatile sig_atomic_t run_threads=1;

static int sock_in;
static int sock_out;

static int core_socket_fd=-1;
static struct privkey deckey;

static int core_init(void);
static int create_core_socket(void);

static void pipe_accept(void)
{
	struct timeval to;
	to.tv_sec=0; to.tv_usec=0;

	fd_set set;
	FD_SET(create_core_socket(), &set);

	int nfds=select(create_core_socket()+1, &set, NULL, NULL, &to);
	if(nfds>0 && FD_ISSET(create_core_socket(), &set))
	{
		int fd=accept(create_core_socket(), NULL, NULL);
		if(fd>0)
		{
#ifndef NDEBUG
			printf("Pipe connection!\n");
#endif
			pipe_add(fd);
		}
	}
}

static int create_core_socket(void)
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

	FILE* selfkey=fopen(selfkeypath(), "r");
	FILE* selfkey_pub=fopen(selfkeypath_pub(), "r");
	if(!selfkey || !selfkey_pub)
	{
		if(selfkey) {fclose(selfkey);selfkey=NULL;}
		if(selfkey_pub) {fclose(selfkey_pub);selfkey_pub=NULL;}
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

	struct config* conf=getconfig();
	if(!conf->size)
	{
		printf("tapi2p needs to be configured before use.\n"
				"Please set your username and port and restart tapi2p.\n");
		return 1;
	}

	if(create_core_socket() == -1) return -1;

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

static int check_peer_key(struct peer* p, char* addr, int newconn, struct config* conf)
{
	struct configitem* ci;
	if((ci=config_find_item(conf, "Key", addr)))
	{
		assert(p);
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
			return -1;
		}
		if(pubkey_load(&p->key, keybuf))
		{
#ifndef NDEBUG
			printf("Invalid or no key found from %s for peer %s\n", keybuf, addr);
#endif
			peer_free(p);
			return -1;
		}
		else
		{
#ifndef NDEBUG
			printf("Key %s loaded for peer %s, address %x\n", keybuf, addr, p);
#endif
		}
		p->isock=newconn;
	}
	else
	{
#ifndef NDEBUG
		printf("No key specified for peer %s\n", addr);
#endif
		peer_free(p);
		return -1;
	}
	return 0;
}

void* connection_thread(void* args)
{
	sem_t* sem=(sem_t*)args;
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
	if(sock_in<0) exit(EXIT_FAILURE);

	fd_set set;
	FD_ZERO(&set);
	sem_post(sem);
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
					printf("Finding port for %s\n", hbuf);
					if((ci=config_find_item(conf, "Port", hbuf)))
					{
						printf("Found port for %s\n", hbuf);
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
						strncpy(p->addr, hbuf, IPV4_MAX);
						p->port=port;
						struct peer* pp;
						/*
						while((pp=peer_next()))
						{
							printf("%s:%u\no:%d\ni:%d\n", pp->addr, pp->port, pp->osock, pp->isock);
						}
						*/
						if((pp=peer_exists(p)))
						{
							peer_remove(p);
							// Remove the newly allocated peer and check if 
							// we have oneway connection with the old one
							if(check_peer_key(pp, hbuf, newconn, conf) == 0)
							{
								peer_updateset(pp);
#ifndef NDEBUG
								printf("Oneway connection to bidirectional for %s:%u\n", hbuf, port);
#endif
							}
							else
							{
#ifndef NDEBUG
								printf("Peer exists\n");
#endif
							}
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
					if(check_peer_key(p, hbuf, newconn, conf))
					{// Key checking failed
						continue;
					}
					p->osock=socket(AF_INET, SOCK_STREAM, 0);
					((struct sockaddr_in*)&addr)->sin_port=htons(p->port);
#ifndef NDEBUG
					printf("Connecting %s:%d\n", hbuf, p->port);
#endif
					if(connect(p->osock, &addr, addrlen))
					{
#ifndef NDEBUG
						printf("%s not connectable, errno: %d\n", hbuf, errno);
#endif
						p->m_connectable=0;
						close(p->osock);
						// Use special number for outgoing socket
						// to tell that we currently have a oneway connection
						p->osock=SOCKET_ONEWAY;
					}
#ifndef NDEBUG
					else printf("Connected successfully!\n");
#endif
					send_event_to_pipes_simple(PeerConnected, p->addr, 0);
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
	sem_t* sem=(sem_t*)args;
	fd_set rset;
	fd_set wset;

	const int BUFLEN=4096;
	char readbuf[BUFLEN];
	sem_post(sem);

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
						send_event_to_pipes_simple(PeerDisconnected, p->addr, 0);
					}
					else
					{// Send received event to core pipe listeners
						size_t datalen;
						unsigned char* data=aes_decrypt_with_key(readbuf, b, &deckey, &datalen);
						evt_t* e=new_event_fromstr(data, p);
						if(e)
						{
							switch(e->type)
							{
								// Event types that core will run listeners for
								case FilePart:
								case RequestFileTransfer:
								{
									event_run_callbacks(e);
								} // Fall through to send event to pipe listeners in any case
								case Message:
								case ListPeers:
								case PeerConnected:
								{
#ifndef NDEBUG
									printf("Got %d bytes of data\n", e->data_len);
#endif
									send_event_to_pipes(e);
									event_free(e);
									break;
								}
							}
						}
						else
						{
							fprintf(stderr, "Failed to construct event from the received data\n");
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

static int connect_to_peers()
{
	struct config* conf=getconfig();
	struct configsection* sect;
	if(sect=config_find_section(conf, "Peers"))
	{
		for(int i=0; i<sect->itemcount; ++i)
		{// Connect to peers
			struct configitem* ci=config_find_item(conf, "Port", sect->items[i]->key);
			if(ci)
			{
				int sock=new_socket(sect->items[i]->key, ci->val);
				if(sock != -1)
				{
					struct peer* p=peer_new();
					strncpy(p->addr, sect->items[i]->key, IPV4_MAX);
					p->osock=sock;
					// Use special number for incoming socket
					// to tell that we currently have a oneway connection
					p->isock=SOCKET_ONEWAY;
					p->port=atoi(ci->val);
					assert(p->port);
					peer_addtoset(p);
#ifndef NDEBUG
					printf("Connected to %s:%s\n", sect->items[i]->key, ci->val);
#endif
				}
			}
			else
			{
				printf("No port specified for peer %s\n", sect->name);
			}
		}
	}
	return 0;
}

void send_data_to_peer(struct peer* p, evt_t* e)
{
	fd_set wset;
	peer_writeset(&wset);
	int nfds=select(write_max()+1, NULL, &wset, NULL, NULL);
	if(nfds>0)
	{
		int fd;
		if(p->osock==SOCKET_ONEWAY)
		{
#ifndef NDEBUG
			printf("Sending to oneway connection!\n");
#endif
			fd=p->isock;
		}
		else fd=p->osock;
		assert(fd >= 0);
		assert(fd != SOCKET_ONEWAY);
		if(FD_ISSET(fd, &wset))
		{
			size_t enclen=0;
			unsigned char* eventdata=event_as_databuffer(e);
			unsigned char* data=aes_encrypt_random_pass(
								eventdata,
								EVENT_LEN(e),
								PW_LEN,
								&p->key,
								&enclen);
			free(eventdata);
			printf("Sending %d bytes to %d\n", enclen, fd);
			if(send(fd, data, enclen, 0) != enclen)
			{
				// TODO: Remove peer on error
				perror("send");
			}
		}
	}
}

void send_to_all(evt_t* e)
{
	struct peer* p;
	while(p=peer_next())
	{
		send_data_to_peer(p, e);
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

	peermanager_init();

	sem_t init_semaphore;
	sem_init(&init_semaphore, 0, 0);

	pthread_t accept_connections;
	pthread_t read_connections;
	pthread_create(&accept_connections, NULL, &connection_thread, &init_semaphore);
	pthread_create(&read_connections, NULL, &read_thread, &init_semaphore);

	sem_wait(&init_semaphore);
	sem_wait(&init_semaphore);

	connect_to_peers();

	printf("Tapi2p core started.\n");
	event_addlistener(Message, &handlemessage, NULL);
	event_addlistener(ListPeers, &handlelistpeers, getconfig());
	event_addlistener(RequestFileTransfer, &handlefiletransfer, NULL);
	event_addlistener(FilePart, &fileparthandler, NULL);
	while(run_threads)
	{
		pipe_accept();
		evt_t* e=poll_event_from_pipes();
		if(e) event_free(e);
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
