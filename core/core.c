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
#include "util.h"
#include "file.h"

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
#include <fcntl.h>
#include <stdbool.h>

volatile sig_atomic_t run_threads=1;

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
	set_nonblocking(fd);
	if(connect(fd, (struct sockaddr*)&u, sizeof(u)))
	{
		if(errno!=EINPROGRESS || errno != EALREADY)
		{
			perror("Connect");
			return -1;
		}
	}
	if(!check_writability(fd))
	{
		fprintf(stderr, "Socket %d connected, but not writable\n", fd);
		return -1;
	}
	set_blocking(fd);

	return fd;
}

static int mkdirp(const char* ppath, mode_t mode)
{
	char path[255];
	strcpy(path, ppath);
	char* p=path;
	while(*p)
	{
		if(*p == '/' && p!=path)
		{
			*p=0;
			if(mkdir(path, mode) && errno != EEXIST)
			{
				return -1;
			}
			*p='/';
		}
		p++;
	}
	if(mkdir(path, mode) && errno != EEXIST)
	{
		return -1;
	}
	return 0;
}

static int core_init(void)
{
	if(mkdirp(basepath(), 0755) == -1 && errno!=EEXIST)
	{
		fprintf(stderr, "Failed to create tapi2p directory!\n");
		return -1;
	}
	if(mkdir(keypath(), 0755) == -1 && errno!=EEXIST)
	{
		fprintf(stderr, "Failed to create tapi2p key directory!\n");
		return -1;
	}
	if(mkdir(metadatapath(), 0755) == -1 && errno!=EEXIST)
	{
		fprintf(stderr, "Failed to create tapi2p metadata directory!\n");
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
				"Waiting for Setup event...\n");
	}
	else
	{
		const char* md="Metadata";
		struct configsection* files=config_find_section(conf, md);
		if(files)
		{
			char sha_str[SHA_DIGEST_LENGTH*2+1];
			memset(sha_str, 0, SHA_DIGEST_LENGTH*2+1);
			for(int i=0; i<files->itemcount; ++i)
			{
				struct configitem* ci=files->items[i];
				struct stat buf;
				char* mdpath=NULL;
				if(ci->val)
				{
					getpath(metadatapath(), ci->val, &mdpath);
				}
				if(!ci->val || stat(mdpath, &buf) == -1)
				{
					printf("Generating metadata for %s\n", ci->key);
					create_metadata_file(ci->key, sha_str);
					config_add(conf, md, ci->key, sha_str);
				}
				if(mdpath) free(mdpath);
				struct configsection* mdci;
				if(!(mdci=config_find_section(conf, ci->val)))
				{
					config_add(conf, ci->val, "Filename", ci->key);
				}
			}
			config_save(conf, configpath());
		}
	}

	if(create_core_socket() == -1) return -1;

	pipe_init();
	return conf->size ? 0 : 1;
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
			printf("Key %s loaded for peer %s\n", keybuf, addr);
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
								pp->m_key_ok=1;
								peer_updateset(pp);
#ifndef NDEBUG
								printf("Oneway connection to bidirectional for %s:%u\n", hbuf, port);
#endif
							}
							else
							{
								pp->m_key_ok=0;
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
						printf("Key checking failed for peer %s\n", hbuf);
						p->m_key_ok=0;
						continue;
					}
					else p->m_key_ok=1;

					p->osock=socket(AF_INET, SOCK_STREAM, 0);
					((struct sockaddr_in*)&addr)->sin_port=htons(p->port);
#ifndef NDEBUG
					printf("Connecting %s:%d\n", hbuf, p->port);
#endif
					set_nonblocking(p->osock);
					if(connect(p->osock, &addr, addrlen))
					{
						int writable=0;
						if(errno==EINPROGRESS || errno == EALREADY)
						{
							writable=check_writability(p->osock);
						}
						if(!writable)
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
						else printf("Connected successfully!\n");
					}
#ifndef NDEBUG
					else printf("Connected successfully!\n");
					set_blocking(p->osock);
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

	uint8_t* databuf=NULL;
	size_t maxdatabuflen=0;
	enc_t encdata;
	encdata.m_Magic=0;
	encdata.m_DataLen=0;
	encdata.m_PassLen=0;
	encdata.m_Data=NULL;
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
				int recvsock=p->isock!=SOCKET_ONEWAY ? p->isock : p->osock;
				if(FD_ISSET(recvsock, &rset))
				{
					ssize_t b=recv(recvsock, &encdata, sizeof(enc_t), 0);
					if(b<=0)
					{
						printf("Removing peer %s:%u\n", p->addr, p->port);
						peer_removefromset(p);
						list_add_node(&delptrs, p);
						send_event_to_pipes_simple(PeerDisconnected, p->addr, 0);
						continue;
					}
					else printf("Received %ld bytes of header\n", b);
					if(encdata.m_Magic != 0x0074B12B)
					{
						printf("Invalid magic!\n");
						continue;
					}
					if(encdata.m_PassLen+encdata.m_DataLen > maxdatabuflen)
					{
						printf("Max datalen now %d, was %d\n", encdata.m_PassLen+encdata.m_DataLen, maxdatabuflen);
						maxdatabuflen=encdata.m_PassLen+encdata.m_DataLen;
						uint8_t* tmp=realloc(databuf, maxdatabuflen);
						databuf=tmp;
					}
					uint32_t received=0;
					while(received < encdata.m_PassLen + encdata.m_DataLen)
					{
						b=recv(recvsock, databuf+received, encdata.m_PassLen+encdata.m_DataLen, 0);
						if(b<=0)
						{
							printf("Removing peer %s:%u\n", p->addr, p->port);
							peer_removefromset(p);
							list_add_node(&delptrs, p);
							send_event_to_pipes_simple(PeerDisconnected, p->addr, 0);
						}
						received+=b;
					}

					// Send received event to core pipe listeners
					printf("Received %lu bytes\n", b);
					encdata.m_Data=databuf;
					size_t datalen;
					evt_t* e=aes_decrypt_with_key(&encdata, &deckey, &datalen);
					e->data=(uint8_t*)e + sizeof(evt_t);
					memcpy(e->addr, p->addr, IPV4_MAX);
					e->port=p->port;
					if(e)
					{
						printf("Event type %d\n", e->type);
						switch(e->type)
						{
							// Event types that core will run listeners for
							case RequestFilePart:
							case RequestFileTransfer:
							case RequestFileTransferLocal:
							case RequestFileList:
							case RequestFileListLocal:
							{
								event_run_callbacks(e);
							} // Fall through to send event to pipe listeners as well
							case Message:
							case ListPeers:
							case PeerConnected:
							case FileList:
							{
#ifndef NDEBUG
								printf("Got %d bytes of data\n", e->data_len);
#endif
								send_event_to_pipes((pipeevt_t*)e);
								break;
							}
							// Event types that won't be sent to pipe listeners
							case Metadata:
							case FilePart:
								event_run_callbacks(e);
								break;
						}
					}
					else
					{
						fprintf(stderr, "Failed to construct event from the received data\n");
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
	return 0;
}

static int connect_to_peers()
{
	struct config* conf=getconfig();
	struct configsection* sect;
	if((sect=config_find_section(conf, "Peers")))
	{
		for(int i=0; i<sect->itemcount; ++i)
		{// Connect to peers
			struct configitem* ci=config_find_item(conf, "Port", sect->items[i]->key);
			if(ci)
			{
				unsigned short port = atoi(ci->val);
				if(port==0 || port>65535)
				{
					fprintf(stderr, "Invalid port number: %u\n", port);
					run_threads=0;
				}
				struct peer* p=peer_exists_simple(sect->items[i]->key, port);
				if(p) continue;
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
					if(check_peer_key(p, sect->items[i]->key, SOCKET_ONEWAY, conf) == 0)
					{
						p->m_key_ok=1;
#ifndef NDEBUG
						printf("Connected to %s:%s\n", sect->items[i]->key, ci->val);
#endif
					}
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

static void send_event_to_peer_internal(struct peer* p, evt_t* e, int nonblocking)
{
	fd_set wset;
	peer_writeset(&wset);
	struct timeval to;
	to.tv_sec=1;
	to.tv_usec=0;
	int nfds=select(write_max()+1, NULL, &wset, NULL, &to);
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
			enc_t* data=aes_encrypt_random_pass(
								(unsigned char*)e,
								sizeof(evt_t) + e->data_len,
								PW_LEN,
								&p->key,
								&enclen);
			printf("Sending %lu bytes to %d, %u bytes decrypted\n", enclen, fd, data->m_DataLen);
			if(nonblocking) set_nonblocking(fd);
			if(send(fd, data, enclen, 0) != enclen)
			{
				if(nonblocking && errno != EINPROGRESS)
				{
					// TODO: Remove peer on error
					perror("send");
				}
				else if(!nonblocking)
				{
					// TODO: Remove peer on error
					perror("send");
				}
			}
			free(data);
			if(nonblocking) set_blocking(fd);
		}
	}
	else
	{
		printf("Send timed out for %s:%u\n", p->addr, p->port);
	}
}

void send_event_to_peer(struct peer* p, evt_t* e)
{
	send_event_to_peer_internal(p, e, 0);
}

void send_event_to_peer_nonblocking(struct peer* p, evt_t* e)
{
	send_event_to_peer_internal(p, e, 1);
}


void send_to_all(evt_t* e)
{
	struct peer* p;
	while((p=peer_next()))
	{
		send_event_to_peer(p, e);
	}
}

void* try_connect_loop(void* args)
{
	while(run_threads)
	{
		sleep(15);
		connect_to_peers();
	}
	return 0;
}

int core_start(void)
{
	run_threads=1;
	if(!setlocale(LC_CTYPE, "en_US.UTF-8"))
	{
		fprintf(stderr, "Cannot set UTF-8 encoding. Please make sure that en_US.UTF-8 encoding is installed.\n");
	}
	int ci=core_init();
	bool setupstatus=false;
	if(ci<0)
	{
		fprintf(stderr, "Tapi2p core failed to initialize!\n");
		return -1;
	}
	else if(ci>0)
	{// Config file created
		pipe_event_addlistener(Setup, &handlesetup, NULL);
		pipe_event_addlistener(Status, &handlestatus, &setupstatus);
		while(run_threads==1)
		{
			pipe_accept();
			pipeevt_t* e=poll_event_from_pipes();
			if(e) free(e);
		}
		pipe_event_removelistener(Setup, &handlesetup);
		if(!run_threads)
		{// If core was exited during setup phase
			return 0;
		}
		run_threads=1;

		// Remove listener during setupstatus change
		pipe_event_removelistener(Status, &handlestatus);
		setupstatus=true;
	}
	else setupstatus=true;

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
	pthread_t try_connecting_to_peers;
	pthread_create(&accept_connections, NULL, &connection_thread, &init_semaphore);
	pthread_create(&read_connections, NULL, &read_thread, &init_semaphore);

	sem_wait(&init_semaphore);
	sem_wait(&init_semaphore);

	connect_to_peers();
	pthread_create(&try_connecting_to_peers, NULL, &try_connect_loop, NULL);

	printf("Tapi2p core started.\n");
	event_addlistener(Message, &handlemessage, NULL);
	pipe_event_addlistener(ListPeers, &handlelistpeers, getconfig());
	pipe_event_addlistener(Status, &handlestatus, &setupstatus);
	event_addlistener(RequestFileTransfer, &handlefiletransfer, NULL);
	event_addlistener(RequestFileTransferLocal, &handlefiletransferlocal, NULL);
	event_addlistener(RequestFilePart, &handlefilepartrequest, NULL);
	event_addlistener(Metadata, &handlemetadata, NULL);
	event_addlistener(FilePart, &handlefilepart, NULL);
	event_addlistener(RequestFileList, &handlerequestfilelist, NULL);
	event_addlistener(RequestFileListLocal, &handlerequestfilelistlocal, NULL);
	event_addlistener(AddFile, &handleaddfile, NULL);

	while(run_threads)
	{
		pipe_accept();
		pipeevt_t* e=poll_event_from_pipes();
		if(e) free(e);
	}

	printf("Tapi2p core shutting down...\n");
	pthread_cancel(try_connecting_to_peers);
	pthread_join(try_connecting_to_peers, NULL);
	pthread_join(accept_connections, NULL);
	pthread_join(read_connections, NULL);
	return 0;
}

void core_stop(void)
{
	run_threads=0;
}
