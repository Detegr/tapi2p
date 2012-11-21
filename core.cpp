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
#include "PipeManager.h"
#include "Event.h"
#include <arpa/inet.h>
#include <sys/un.h>
#include <signal.h>

using namespace dtglib;

RSA_PrivateKey pkey;
bool run_threads=true;
int core_fd;

void startup_init(const char* custompath=NULL);
void generate_self_keypair(Config& c, const std::string kp);

int setup_local_socket(const std::string& name)
{
	struct sockaddr_un u;
	unlink(name.c_str());
	int fd=socket(AF_UNIX, SOCK_STREAM, 0);
	if(fd<=0)
	{
		std::cerr << "Error creating unix socket!" << std::endl;
		return -1;
	}
	memset(&u, 0, sizeof(struct sockaddr_un));
	u.sun_family=AF_UNIX;
	strncpy(u.sun_path, name.c_str(), sizeof(u.sun_path)-1);
	if(bind(fd, (struct sockaddr*)&u, sizeof(u)) == -1)
	{
		std::cerr << "Failed to bind unix socket" << std::endl;
		unlink(name.c_str());
		return -1;
	}
	if(listen(fd, 10) == -1)
	{
		std::cerr << "Failed to listen unix socket" << std::endl;
		unlink(name.c_str());
		return -1;
	}
	return fd;
}

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
	core_fd=setup_local_socket(tapipath + "/t2p_core");
	if(core_fd==-1)
	{
		exit(EXIT_FAILURE);
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
		unsigned char* np=(unsigned char*)memmem(p.M_RawData()+AES::MAGIC_LEN, p.M_Size()-AES::MAGIC_LEN, AES::Magic(), AES::MAGIC_LEN);
		if(np)
		{
			while(np)
			{
				std::vector<unsigned char>& data=AES::Decrypt((unsigned char*)p.M_RawData(), np-p.M_RawData(), pkey);
				p.M_Pop(np-p.M_RawData());
				std::wcout << L"1" << (wchar_t*)&data[0] << std::endl;
				//tapi2p::UI::WriteLine(tapi2p::UI::Main(), L"[" + nick + L"] " + (wchar_t*)&data[0]);
				np=(unsigned char*)memmem(p.M_RawData()+AES::MAGIC_LEN, p.M_Size()-AES::MAGIC_LEN, AES::Magic(), AES::MAGIC_LEN);
			}
			std::vector<unsigned char>& data=AES::Decrypt((unsigned char*)p.M_RawData(), p.M_Size(), pkey);
			std::wcout << L"2" << (wchar_t*)&data[0] << std::endl;
			//tapi2p::UI::WriteLine(tapi2p::UI::Main(), L"[" + nick + L"] " + (wchar_t*)&data[0]);
		}
		else
		{
			std::vector<unsigned char>& data=AES::Decrypt((unsigned char*)p.M_RawData(), p.M_Size(), pkey);
			
			const std::vector<int>& fds=PipeManager::Container();
			std::vector<int> disconnected;
			PipeManager::Lock();
			for(std::vector<int>::const_iterator it=fds.begin(); it!=fds.end(); ++it)
			{
				int b=send(*it, (char*)&data[0], data.size(), 0);
				if(b<=0)
				{
					disconnected.push_back(*it);
				}
			}
			PipeManager::Unlock();
			for(std::vector<int>::const_iterator it=disconnected.begin(); it!=disconnected.end(); ++it)
			{
				PipeManager::Remove(*it);
			}
			//std::wcout << (wchar_t*)&data[0] << std::endl;
			//tapi2p::UI::WriteLine(tapi2p::UI::Main(), L"[" + nick + L"] " + (wchar_t*)&data[0]);
		}
	}
	catch(KeyException& e)
	{
		//tapi2p::UI::WriteLine(tapi2p::UI::Main(), L"Failed to decrypt incoming message from: " + nick);
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
							else if((*pt)->Sock_Out.M_Ip() == sock->M_Ip() && (*pt)->Sock_Out.M_Port() == port)
							{// One-way to bi-directional
								(*pt)->Sock_In = *sock;
								(*pt)->m_Selector.M_Add((*pt)->Sock_In);
								newconn=false;
								break;
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
	C_Selector s;
	const std::vector<Peer*>& peers=PeerManager::Do();
	C_Packet p;
	for(std::vector<Peer*>::const_iterator it=peers.begin(); it!=peers.end(); ++it)
	{
		std::vector<unsigned char>& data=AES::Encrypt((unsigned char*)msg.c_str(), (msg.size()*sizeof(wchar_t))+sizeof(wchar_t), 80, (*it)->Key);
		for(int i=0; i<data.size(); ++i) p << data[i];
		if((*it)->m_Connectable)
		{
			s.M_Add((*it)->Sock_Out);
			s.M_WaitWrite(1000);
			if(s.M_IsReady((*it)->Sock_Out))
			{
				(*it)->Sock_Out.M_Send(p);
			}
			//else tapi2p::UI::WriteLine(tapi2p::UI::Active(), L"Failed to write sockout");
		}
		else
		{
			s.M_Add((*it)->Sock_In);
			s.M_WaitWrite(1000);
			if(s.M_IsReady((*it)->Sock_In))
			{
				(*it)->Sock_In.M_Send(p);
			}
			//else tapi2p::UI::WriteLine(tapi2p::UI::Active(), L"Failed to write sockin");
		}
		s.M_Clear();
		p.M_Clear();
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
	//tapi2p::UI::Write(tapi2p::UI::Content, L"Trying: " + ipw + L":" + portstr);
	try
	{
		C_TcpSocket sock(ci.Key().c_str(), port);

		// We need to set the socket to nonblocking mode to not get blocked
		// if we try to connect to a peer which is not online at the moment.
		int sockargs=fcntl(sock.M_Fd(), F_GETFL, NULL);
		sockargs |= O_NONBLOCK;
		if(fcntl(sock.M_Fd(), F_SETFL, sockargs)<0)
		{
			//tapi2p::UI::Write(tapi2p::UI::Content, L"Socket error.");
		}
		sock.M_Connect();
		sockargs &= ~O_NONBLOCK;
		if(fcntl(sock.M_Fd(), F_SETFL, sockargs)<0)
		{
			//tapi2p::UI::Write(tapi2p::UI::Content, L"Socket error.");
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
				//tapi2p::UI::Write(tapi2p::UI::Content, L"Fail.");
				return;
			}
			//tapi2p::UI::Write(tapi2p::UI::Content, L"Connected.");
			p = new Peer();
			p->Sock_Out=sock;
			p->Key.Load(PathManager::KeyPath() + "/" + c.Get(ci.Key(), "Key"));
			PeerManager::Add(p);
			p->Thread.M_Start(peerloop, p);
		}
	}
	catch(const std::runtime_error& e)
	{
		//tapi2p::UI::Write(tapi2p::UI::Content, L"Fail");
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

void pipe_accept(void*)
{
	struct timeval to;
	fd_set orig;
	FD_SET(core_fd, &orig);
	fd_set set;
	while(run_threads)
	{
		set=orig;
		to.tv_sec=1;
		to.tv_usec=0;
		int nfds=select(core_fd+1, &set, NULL, NULL, &to);
		if(nfds>0 && FD_ISSET(core_fd, &set))
		{
			int fd=accept(core_fd, NULL, NULL);
			if(fd>0) PipeManager::Add(fd);
		}
	}
}

int main(int argc, char** argv)
{
	signal(SIGPIPE, SIG_IGN); // We don't need SIGPIPE when AF_UNIX socket is disconnected.
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

	C_Thread network_thread(&network_startup);
	C_Thread connection_thread(connect_to_peers);
	C_Thread pipe_thread(pipe_accept);

	Config& c = PathManager::GetConfig();
	while(run_threads)
	{
		//Event e=poll_event();
		sleep(1);
		sendall(L"foobar");
	}
	connection_thread.M_Join();
	network_thread.M_Join();
	PathManager::Destroy();
}
