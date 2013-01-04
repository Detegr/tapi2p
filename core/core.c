#include "core.h"
#include "../crypt/publickey.h"
#include "../crypt/keygen.h"
#include "pathmanager.h"
#include "pipemanager.h"
#include "config.h"
#include "event.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <locale.h>
#include <unistd.h>

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
		if(fd>0) pipe_add(fd);
	}
}

int core_socket(void)
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
		FILE* f=fopen(configpath(), "w");
		config_flush(conf, f);
		fclose(f);
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
		printf("Selfkey not found, generating...\n");
		if(generate(selfkeypath(), T2PPRIVATEKEY|T2PPUBLICKEY) == -1)
		{
			fprintf(stderr, "Failed to create keypair!\n");
			return -1;
		}
		printf("OK!\n");
	}
	fclose(selfkey);
	fclose(selfkey_pub);

	if(core_socket() == -1)
	{
		return -1;
	}

	pipe_init();

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

	while(run_threads)
	{
		pipe_accept();
		struct Event* e=poll_event();
		if(e) event_free(e);
	}
	
	printf("Tapi2p core shutting down...\n");
	return 0;
}

void core_stop(void)
{
	run_threads=0;
}
