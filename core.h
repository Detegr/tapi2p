#include "AES.h"
#include "Config.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include "PrivateKey.h"
#include "PublicKey.h"
#include "KeyGenerator.h"
#include "PathManager.h"
#include "PeerManager.h"
#include "PipeManager.h"
#include "Event.h"

class Core
{
	private:
		static RSA_PrivateKey pkey;
		static bool run_threads;
		static int core_fd;

		static void startup_init(const char* custompath=NULL);
		static int setup_local_socket(const std::string& name);
	public:
		static void ParsePacket(C_Packet& p, const std::wstring& nick);
		static int Start(int argc, char** argv);
		static void Stop();
		static bool Running() { return run_threads; }
		static int Socket() { return core_fd; }
};
