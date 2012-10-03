#include "AES.h"
#include "Config.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

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
		std::string nick;
		std::cout << "Seems like this is the first time you run Tapi2P\nPlease input your nickname:" << std::endl;
		std::cin >> nick;
		conf.Set("Account", "Nick", nick);
		conf.Flush();
	}
	else
	{
		std::cout << "Welcome to Tapi2P, " << conf.Get("Account", "Nick") << std::endl;
	}
}

int main(int argc, char** argv)
{
	startup_init();
}
