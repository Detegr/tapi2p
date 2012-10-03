#include "AES.h"
#include "Config.h"
#include "dtglib/Network.h"
#include "dtglib/Concurrency.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

using namespace dtglib;

void startup_init()
{
	struct passwd* pw = getpwuid(getuid());
	std::string tapipath=pw->pw_dir;
	tapipath += "/.tapi2p";
	mkdir(tapipath.c_str(), 0755);
	tapipath+="/config";
	Config conf(tapipath);
	if(!conf.Size())
	{
		std::string data;
		std::cout << "Seems like this is the first time you run Tapi2P\nPlease input your nickname:" << std::endl;
		std::cin >> data;
		conf.Set("Account", "Nick", data);
		std::cout << "Which port would you like to use?\nNote: This port needs to be open and routed to your computer in order for Tapi2P to work correctly." << std::endl;
		std::cin >> data;
		// TODO: Check that the port is actually in valid port range
		conf.Set("Account", "Port", data);
		conf.Flush();
	}
	else
	{
		std::cout << "Welcome to Tapi2P, " << conf.Get("Account", "Nick") << std::endl;
	}
}

void network_startup(void* args)
{
	std::cout << "Network starting up..." << std::endl;
	sleep(5);
	return;
}

int main(int argc, char** argv)
{
	startup_init();
	C_Thread network_thread(&network_startup);
	network_thread.M_Join();
}
