#ifndef TAPI2P_CORE_H
#define TAPI2P_CORE_H

#include "../crypt/publickey.h"

static int core_socket_fd=-1;
static int run_threads;
static struct pubkey pkey;

int core_start(int argc, char** argv);
void core_stop(void);
int core_socket(void);

//#include "aes.h"
//#include "privatekey.h"
//#include "publickey.h"
//#include "keygenerator.h"
//#include "pathmanager.h"
//#include "peermanager.h"
//#include "pipemanager.h"
//#include "config.h"
//#include <sys/stat.h>
//#include <sys/types.h>
//#include <unistd.h>
//#include <pwd.h>
//#include "event.h"
//
//class Core
//{
//	private:
//		static RSA_PrivateKey pkey;
//		static bool run_threads;
//		static int core_fd;
//
//		static void startup_init(const char* custompath=NULL);
//		static int setup_local_socket(const std::string& name);
//	public:
//		static void ParsePacket(C_Packet& p, const std::wstring& nick);
//		static int Start(int argc, char** argv);
//		static void Stop();
//		static bool Running() { return run_threads; }
//		static int Socket() { return core_fd; }
//};

#endif
