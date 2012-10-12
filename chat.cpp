#include "AES.h"
#include "Config.h"
#include "dtglib/Network.h"
#include "dtglib/Concurrency.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include "PrivateKey.h"
#include "PublicKey.h"
#include "KeyGenerator.h"
#include "PathManager.h"
#include "PeerManager.h"

using namespace dtglib;
RSA_PrivateKey pkey;
bool run_threads=true;

void startup_init(const char* custompath=NULL);
void generate_self_keypair(Config& c, const std::string kp);

void startup_init(const char* custompath)
{
	std::string tapipath;
	struct passwd* pw=NULL;
	if(!custompath)
	{
		pw = getpwuid(getuid());
		tapipath=pw->pw_dir;
		tapipath += "/.tapi2p";
	}
	else tapipath=custompath;
	PathManager::Setup(tapipath);
	mkdir(tapipath.c_str(), 0755);
	mkdir(PathManager::KeyPath().c_str(), 0755);
	Config conf(PathManager::ConfigPath());
	if(!conf.Size())
	{
		std::string data;
		std::cout << "Seems like this is the first time you run Tapi2P\nPlease input your nickname:" << std::endl;
		std::getline(std::cin, data);
		conf.Set("Account", "Nick", data);
		std::cout << "Which port would you like to use?\nNote: This port needs to be open and routed to your computer in order for Tapi2P to work correctly." << std::endl;
		std::getline(std::cin, data);
		// TODO: Check that the port is actually in valid port range
		conf.Set("Account", "Port", data);
		conf.Flush();
	}
	else
	{
		std::cout << "Welcome to tapi2p, " << conf.Get("Account", "Nick") << std::endl;
	}
	std::ifstream selfkey((PathManager::SelfKeyPath()+".pub").c_str());
	if(selfkey.is_open()) selfkey.close();
	else
	{
		generate_self_keypair(conf,PathManager::SelfKeyPath());
	}
}

void generate_self_keypair(Config& c, const std::string kp)
{
	std::cout << "Generating RSA Keypair...This might take a while" << std::endl;
	RSA_KeyGenerator::WriteKeyPair(kp);
	RSA_PublicKey pk;
	try
	{
		pk.Load(kp);
		std::cout << "Keypair created.\nGive this public key to your peers:\n\n" << pk << std::endl;
	}
	catch(KeyException& e)
	{
		std::cout << "Keypair creation failed.\nReason: " << e.what() << std::endl;
	}
}

static void peerloop(void* arg)
{
	Peer* p = (Peer*)arg;
	C_Selector s;
	s.M_Add(*p->Sock_In);
	while(run_threads)
	{
		s.M_Wait(1000);
		if(s.M_IsReady(*p->Sock_In))
		{
			if(p->Sock_In->M_Receive(p->Packet, 1000))
			{
				try
				{
					std::vector<unsigned char>& data=AES::Decrypt((unsigned char*)p->Packet.M_RawData(), p->Packet.M_Size(), pkey);
					std::cout << (char*)&data[0] << std::flush;
				}
				catch(KeyException& e)
				{
					std::cout << "Failed to decrypt incoming message!" << std::endl;
				}
				p->Packet.M_Clear();
			}
			else
			{
				std::cout << "Connection closed." << std::endl;
				PeerManager::Remove(p);
				break;
			}
		}
	}
	return;
}

void network_startup(void* args)
{
	Config c(PathManager::ConfigPath());

	unsigned short port;
	std::stringstream ss;
	ss << c.Get("Account", "Port");
	ss >> port;
	std::cout << "Using port " << port << std::endl;
	C_TcpSocket m_Incoming(port);
	m_Incoming.M_Bind();
	m_Incoming.M_Listen();
	C_Selector s;
	s.M_Add(m_Incoming);
	C_Packet p;

	std::vector<ConfigItem> known_peers=c.Get("Peers");
	while(run_threads)
	{
		s.M_Wait(2000);
		if(s.M_IsReady(m_Incoming))
		{
			C_TcpSocket* sock=m_Incoming.M_Accept();
			if(sock)
			{
				Peer* p=NULL;
				for(std::vector<ConfigItem>::const_iterator it=known_peers.begin(); it!=known_peers.end(); ++it)
				{
					if(sock->M_Ip() == it->Key().c_str())
					{
						bool newconn=true;
						unsigned short port;
						std::string portstr=c.Get(it->Key(), "Port");
						std::stringstream ss;
						ss << portstr; ss >> port;

						std::vector<Peer*>& peers=PeerManager::Do();
						for(std::vector<Peer*>::iterator pt=peers.begin(); pt!=peers.end(); ++pt)
						{
							// If Sock_In!=NULL and ips and ports match, we have an existing connection
							if((*pt)->Sock_In && (*pt)->Sock_In->M_Ip() == sock->M_Ip() && (*pt)->Sock_Out.M_Port() == port) newconn=false;
							// If Sock_Out ip matches the current ip, and the ports match, we have an connection from out-only connection, so we make it bidirectional
							else if((*pt)->Sock_Out.M_Ip() == sock->M_Ip() && (*pt)->Sock_Out.M_Port() == port)
							{
								(*pt)->Sock_In = sock;
								newconn=false;
								(*pt)->Thread.M_Start(peerloop, *pt);
								break;
							}
						}
						PeerManager::Done();

						if(newconn)
						{
							p = new Peer(sock);
							try
							{
								p->Key.Load(PathManager::KeyPath() + "/" + c.Get(it->Key(), "Key"));
							}
							catch(const KeyException& e)
							{
								std::cout << "Peer identification failed: " << e.what() << std::endl;
								delete p;
								p=NULL;
							}
							if(p)
							{
								try
								{
									std::cout << sock->M_Ip() << ":" << port << std::endl;
									p->Sock_Out=C_TcpSocket(sock->M_Ip(), port);
									p->Sock_Out.M_Connect();
								}
								catch(const std::runtime_error& e)
								{
									std::cout << "!!! One-way connection detected" << std::endl;
									p->m_Connectable=false;
									p->Sock_Out.M_Close();
								}
							}
						}
						break;
					}
				}
				if(p)
				{
					PeerManager::Add(p);
					p->Thread.M_Start(peerloop, p);
				}
			}
		}
	}
	m_Incoming.M_Close();
	return;
}
void sendall(const std::string& msg)
{
	const std::vector<Peer*>& peers=PeerManager::Do();
	for(std::vector<Peer*>::const_iterator it=peers.begin(); it!=peers.end(); ++it)
	{
		std::vector<unsigned char>& data=AES::Encrypt((unsigned char*)msg.c_str(), msg.size()+1, "passwd", (*it)->Key);
		C_Packet p;
		for(int i=0; i<data.size(); ++i) p << data[i];
		if((*it)->m_Connectable)
		{
			(*it)->Sock_Out.M_Send(p);
		} else std::cout << "Not connectable" << std::endl;
	}
	PeerManager::Done();
}

int main(int argc, char** argv)
{
	if(argc>1)
	{
		startup_init(argv[1]);
	}
	else startup_init();
	try
	{
		pkey.Load(PathManager::SelfKeyPath());
	}
	catch(KeyException& e)
	{
		std::cout << "Failed to start tapi2p!" << std::endl;
		std::cout << e.what() << std::endl;
		return 1;
	}
	C_Thread network_thread(&network_startup);
	std::string cmd;
	Config c(PathManager::ConfigPath());
	while(run_threads)
	{
		std::cout << "tapi2p> ";
		std::getline(std::cin, cmd);
		if(cmd=="q" || cmd=="quit") run_threads=false;
		else if(cmd==":add" || cmd==":a")
		{
			cmd.clear();
			std::cout << "Input peer ip" << std::endl;
			std::getline(std::cin, cmd);
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
				std::cout << "Input peer's port" << std::endl;
				std::getline(std::cin, cmd);
				c.Set("Peers", ip.M_ToString());
				c.Set(ip.M_ToString(), "Port", cmd);
				std::cout << "Input peer key file name" << std::endl;
				std::getline(std::cin, cmd);
				c.Set(ip.M_ToString(), "Key", cmd);
				std::cout << "Give nickname to peer" << std::endl;
				std::getline(std::cin, cmd);
				c.Set(ip.M_ToString(), "Nick", cmd);
				c.Flush();
				std::cout << "----\n" << cmd << " added successfully.\nIp: " << ip.M_ToString() << ":" << c.Get(ip.M_ToString(), "Port") << "\nKey file: " << c.Get(ip.M_ToString(), "Key") << "\n----" << std::endl;
			}
		}
		else if(cmd==":peers" || cmd==":p")
		{
			std::vector<ConfigItem> peers=c.Get("Peers");
			for(std::vector<ConfigItem>::const_iterator it=peers.begin(); it!=peers.end(); ++it)
			{
				std::cout << c.Get(it->Key(), "Nick") << ": " << it->Key() << std::endl;
			}
		}
		else if(cmd==":c" || cmd==":connect")
		{
			std::vector<ConfigItem> peerconfs=c.Get("Peers");
			for(std::vector<ConfigItem>::const_iterator it=peerconfs.begin(); it!=peerconfs.end(); ++it)
			{
				unsigned short port;
				std::stringstream ss;
				std::string portstr=c.Get(it->Key(), "Port");
				ss << portstr; ss >> port;
				Peer* p=NULL;
				try
				{
					std::cout << "Trying " << it->Key() << ":" << port << std::endl;
					C_TcpSocket sock(it->Key().c_str(), port);
					int yes=1;
					setsockopt(sock.M_Fd(), SOL_SOCKET, O_NONBLOCK, (char*)&yes, sizeof(yes));
					sock.M_Connect();
					yes=0;
					setsockopt(sock.M_Fd(), SOL_SOCKET, O_NONBLOCK, (char*)&yes, sizeof(yes));
					p = new Peer(NULL);
					p->Sock_Out=sock;
					p->Key.Load(PathManager::KeyPath() + "/" + c.Get(it->Key(), "Key"));
					PeerManager::Add(p);
				}
				catch(const std::runtime_error& e)
				{
					std::cout << e.what() << std::endl;
					if(p)
					{
						std::cout << "foo" << std::endl;
						p->Sock_Out.M_Close();
						PeerManager::Remove(p);
					}
				}
				catch(const KeyException& e)
				{
					std::cout << e.what() << std::endl;
					if(p)
					{
						p->Sock_Out.M_Close();
						PeerManager::Remove(p);
					}
				}
			}
		}
		else
		{
			sendall(cmd);
		}
	}
	network_thread.M_Join();
}
