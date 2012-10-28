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
#include "ui.h"

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

void parsepacket(C_Packet& p, const std::wstring& nick)
{
	try
	{
		std::vector<unsigned char>& data=AES::Decrypt((unsigned char*)p.M_RawData(), p.M_Size(), pkey);
		tapi2p::UI::Write(tapi2p::UI::Content, L"[" + nick + L"] " + (wchar_t*)&data[0]);
	}
	catch(KeyException& e)
	{
		tapi2p::UI::Write(tapi2p::UI::Content, L"Failed to decrypt incoming message from: " + nick);
	}
	p.M_Clear();
}

static void peerloop(void* arg)
{
	Peer* p = (Peer*)arg;
	p->m_Selector.M_Add(p->Sock_In);
	p->m_Selector.M_Add(p->Sock_Out);
	Config& c = PathManager::GetConfig();
	std::wstring nick;
	if(p->Sock_In.M_Fd()>0) nick=c.Getw(p->Sock_In.M_Ip().M_ToString(), "Nick");
	else nick=c.Getw(p->Sock_Out.M_Ip().M_ToString(), "Nick");
	while(run_threads)
	{
		p->m_Selector.M_Wait(1000);
		if(p->Sock_In.M_Fd()>0 && p->m_Selector.M_IsReady(p->Sock_In))
		{
			if(p->Sock_In.M_Receive(p->Packet, 1000))
			{
				parsepacket(p->Packet,nick);
			}
			else
			{
				PeerManager::Remove(p);
				tapi2p::UI::Update();
				break;
			}
		}
		else if(p->Sock_Out.M_Fd()>0 && p->m_Selector.M_IsReady(p->Sock_Out))
		{
			if(p->Sock_Out.M_Receive(p->Packet, 1000))
			{
				parsepacket(p->Packet,nick);
			}
			else
			{
				PeerManager::Remove(p);
				tapi2p::UI::Update();
				break;
			}
		}
	}
	return;
}

void network_startup(void* args)
{
	Config& c = PathManager::GetConfig();

	unsigned short port;
	std::stringstream ss;
	ss << c.Get("Account", "Port");
	ss >> port;
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
							if((*pt)->Sock_In.M_Fd() && (*pt)->Sock_In.M_Ip() == sock->M_Ip() && (*pt)->Sock_Out.M_Port() == port)
							{
								newconn=false;
								break;
							}
							else if((*pt)->Sock_Out.M_Ip() == sock->M_Ip() && (*pt)->Sock_Out.M_Port() == port && (!(*pt)->m_Connectable))
							{
								newconn=false;
								//(*pt)->Thread.M_Start(peerloop, *pt);
								//tapi2p::UI::Update();
								break;
							}
							else
							{
								(*pt)->Sock_In = *sock;
								(*pt)->m_Selector.M_Add((*pt)->Sock_In);
								newconn=false;
							}
						}
						PeerManager::Done();

						if(newconn)
						{
							p = new Peer(*sock);
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
									//std::cout << sock->M_Ip() << ":" << port << std::endl;
									p->Sock_Out=C_TcpSocket(sock->M_Ip(), port);
									p->Sock_Out.M_Connect();
								}
								catch(const std::runtime_error& e)
								{
									p->m_Connectable=false;
									p->Sock_Out.M_Disconnect();
									p->Sock_Out.M_Clear();
								}

								PeerManager::Add(p);
								p->Thread.M_Start(peerloop, p);
							}
						}
						tapi2p::UI::Update();
						break;
					}
				}
			}
		}
	}
	m_Incoming.M_Close();
	return;
}
void sendall(const std::wstring& msg)
{
	const std::vector<Peer*>& peers=PeerManager::Do();
	for(std::vector<Peer*>::const_iterator it=peers.begin(); it!=peers.end(); ++it)
	{
		std::vector<unsigned char>& data=AES::Encrypt((unsigned char*)msg.c_str(), msg.size()*sizeof(wchar_t)+sizeof(wchar_t), 80, (*it)->Key);
		C_Packet p;
		for(int i=0; i<data.size(); ++i) p << data[i];
		if((*it)->m_Connectable) (*it)->Sock_Out.M_Send(p);
		else (*it)->Sock_In.M_Send(p);
	}
	PeerManager::Done();
}

void connect(void* arg)
{
	Config& c = PathManager::GetConfig();
	const ConfigItem& ci=*(const ConfigItem*)arg;
	unsigned short port;
	std::wstringstream ss;
	std::string ip=ci.Key();
	std::wstring portstr=c.Getw(ci.Key(), "Port");
	ss << portstr; ss >> port;
	Peer* p=NULL;
	std::wstring ipw;
	ipw.resize(ip.length());
	std::copy(ip.begin(), ip.end(), ipw.begin());
	tapi2p::UI::Write(tapi2p::UI::Content, L"Trying: " + ipw + L":" + portstr);
	try
	{
		C_TcpSocket sock(ci.Key().c_str(), port);

		// We need to set the socket to nonblocking mode to not get blocked
		// if we try to connect to a peer which is not online at the moment.
		int sockargs=fcntl(sock.M_Fd(), F_GETFL, NULL);
		sockargs |= O_NONBLOCK;
		if(fcntl(sock.M_Fd(), F_SETFL, sockargs)<0)
		{
			tapi2p::UI::Write(tapi2p::UI::Content, L"Socket error.");
		}
		sock.M_Connect();
		sockargs &= ~O_NONBLOCK;
		if(fcntl(sock.M_Fd(), F_SETFL, sockargs)<0)
		{
			tapi2p::UI::Write(tapi2p::UI::Content, L"Socket error.");
		}
		C_Selector s;
		s.M_Add(sock);
		s.M_WaitWrite(5000);
		if(s.M_IsReady(sock))
		{
			int err=0;
			int errlen=sizeof(err);
			if(getsockopt(sock.M_Fd(), SOL_SOCKET, SO_ERROR, (void*)&err, (socklen_t*)&errlen)<0 || err)
			{
				tapi2p::UI::Write(tapi2p::UI::Content, L"Fail.");
				return;
			}
			tapi2p::UI::Write(tapi2p::UI::Content, L"Connected.");
			p = new Peer();
			p->Sock_Out=sock;
			p->Key.Load(PathManager::KeyPath() + "/" + c.Get(ci.Key(), "Key"));
			PeerManager::Add(p);
			p->Thread.M_Start(peerloop, p);
		}
	}
	catch(const std::runtime_error& e)
	{
		tapi2p::UI::Write(tapi2p::UI::Content, L"Fail");
		if(p)
		{
			p->Sock_Out.M_Disconnect();
			PeerManager::Remove(p);
		}
	}
	catch(const KeyException& e)
	{
		std::cout << e.what() << std::endl;
		if(p)
		{
			p->Sock_Out.M_Disconnect();
			PeerManager::Remove(p);
		}
	}
	tapi2p::UI::Update();
}

void connect_to_peers(void*)
{
	Config& c = PathManager::GetConfig();
	std::vector<ConfigItem> peerconfs=c.Get("Peers");
	std::vector<C_Thread*> connections;
	for(std::vector<ConfigItem>::const_iterator it=peerconfs.begin(); it!=peerconfs.end(); ++it)
	{
		connections.push_back(new C_Thread(connect, (void*)&*it));
	}
	for(std::vector<C_Thread*>::const_iterator it=connections.begin(); it!=connections.end(); ++it)
	{
		(*it)->M_Join();
		delete *it;
	}
}

int main(int argc, char** argv)
{
	if(!setlocale(LC_CTYPE, ""))
	{
		std::cerr << "Cannot set specified locale!" << std::endl;
	}
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
	tapi2p::UI::Init();

	C_Thread network_thread(&network_startup);
	C_Thread connection_thread(connect_to_peers);

	Config& c = PathManager::GetConfig();
	while(run_threads)
	{
		std::wstring cmd=tapi2p::UI::HandleInput();
		if(cmd==L"") continue;
		if(cmd==L":q" || cmd==L":quit") run_threads=false;
		/*
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
		*/
		else if(cmd==L":c" || cmd==L":connect")// || cmd==":connect")
		{
			//connect_to_peers(NULL);
		}
		else if(cmd==L":u" || cmd==L":update")
		{
			tapi2p::UI::Update();
		}
		else
		{
			tapi2p::UI::Write(tapi2p::UI::Content, L"[" + c.Getw("Account", "Nick") + L"] " + cmd);
			sendall(cmd);
		}
	}
	connection_thread.M_Join();
	network_thread.M_Join();
	tapi2p::UI::Destroy();
	PathManager::Destroy();
}
