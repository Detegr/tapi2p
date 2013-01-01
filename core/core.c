#include "core.h"
#include "../crypt/publickey.h"
#include "../crypt/keygen.h"
#include "pathmanager.h"
#include <sys/stat.h>
#include <sys/types.h>

int pipe_accept(void* args)
{
	struct timeval to;
	fd_set orig;
	FD_SET(core_socket(), &orig);
	fd_set set;
	while(run_threads)
	{
		memcpy(&set, &orig, sizeof(fd_set));
		to.tv_sec=1;
		to.tv_usec=0;
		int nfds=select(core_socket()+1, &set, NULL, NULL, &to);
		if(nfds>0 && FD_ISSET(core_socket(), &set))
		{
			int fd=accept(core_socket(), NULL, NULL);
			if(fd>0) FD_SET(fd, &pipeset);
		}
	}
}

int core_socket(void)
{
	if(core_socket_fd != -1)
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

static int core_init(const char* custompath)
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
	FILE* selfkey=fopen(selfkeypath_pub(), "r");
	if(!selfkey)
	{
		if(generate(selfkeypath_pub(), T2PPRIVATEKEY|T2PPUBLICKEY) == -1)
		{
			fprintf(stderr, "Failed to create keypair!\n");
			return -1;
		}
	}
	if(core_socket() == -1)
	{
		return -1;
	}
	return 0;
}

int core_start(int argc, char** argv)
{
	run_threads=1;
	if(!setlocale(LC_CTYPE, "en_US.UTF-8")) // Use client's native encoding
	{
		fprintf(stderr, "Cannot set UTF-8 encoding. Please make sure that en_US.UTF-8 encoding is installed.\n");
	}
	if(argc>1)
	{
		core_init(argv[1]);
	} else core_init();
	if(pubkey_load(&pkey, selfkeypath()))
	{
		fprintf(stderr, "Failed to start tapi2p! Client's public key failed to load.\n");
		return -1;
	}

	pthread_t pipethread;
	pthread_create(&pipethread, NULL, pipe_accept, NULL);

	while(run_threads)
	{

	}
	pthread_join(&pipethread, NULL);
}

void core_stop(void)
{
	run_threads=0;
}
