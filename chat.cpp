#include "AES.h"
#include "Config.h"
#include "dtglib/Network.h"
#include "dtglib/Concurrency.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

using namespace dtglib;
bool run_threads=true;

std::string startup_init()
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
	return tapipath;
}

void network_startup(void* args)
{
	std::string confpath=*(std::string*)args;
	Config c(confpath);
	unsigned short port;
	std::stringstream ss;
	ss << c.Get("Account", "Port");
	ss >> port;
	std::cout << "Using port " << port << std::endl;
	C_TcpSocket m_Incoming(port);
	m_Incoming.M_Accept();
	C_Selector s;
	s.M_Add(m_Incoming);
	C_Packet p;
	while(run_threads)
	{
		s.M_Wait(2000);
		if(s.M_IsReady(m_Incoming))
		{
			if(m_Incoming.M_Receive(p))
			{
			}
		}
	}
	m_Incoming.M_Close();
	return;
}

int main(int argc, char** argv)
{
	std::string confpath=startup_init();
	C_Thread network_thread(&network_startup, &confpath);
	std::string cmd;
	Config c(confpath);
	while(run_threads)
	{
		std::cin >> cmd;
		if(cmd=="q") run_threads=false;
		else if(cmd=="add")
		{
			cmd.clear();
			std::cout << "Input peer ip" << std::endl;
			std::cin >> cmd;
			bool ok=true;
			C_IpAddress ip;
			try
			{
				ip=cmd.c_str();
			}
			catch(...)
			{
				std::cout << "Invalid ip" << std::endl;
				ok=false;
			}
			if(ok)
			{
				cmd.clear();
				std::cout << "Input peer key file name" << std::endl;
				std::cin >> cmd;
				c.Set("Peers", ip.M_ToString());
				c.Set(ip.M_ToString(), "Key", cmd);
				std::cout << "Give nickname to peer" << std::endl;
				std::cin >> cmd;
				c.Set(ip.M_ToString(), "Nick", cmd);
				c.Flush();
				std::cout << "----\n" << cmd << " added successfully.\nIp: " << ip.M_ToString() << "\nKey file: " << c.Get(ip.M_ToString(), "Key") << "\n----" << std::endl;
			}
		}
		else if(cmd=="peers")
		{
			std::vector<ConfigItem> peers=c.Get("Peers");
			for(std::vector<ConfigItem>::const_iterator it=peers.begin(); it!=peers.end(); ++it)
			{
				std::cout << c.Get(it->Key(), "Nick") << ": " << it->Key() << std::endl;
			}
		}
	}
	network_thread.M_Join();
}
