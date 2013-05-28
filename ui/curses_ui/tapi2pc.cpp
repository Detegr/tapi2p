#include "curses_ui.h"
#include "peer.h"
#include <functional>
#include <unordered_map>
extern "C"
{
	#include "core/core.h"
	#include "core/pathmanager.h"
}

static bool running=true;
static const std::unordered_map<std::wstring, std::wstring> long_commands=
{
	{L":quit",		L":q"},
	{L":update",	L":u"},
	{L":deltab",	L":d"},
	{L":tabnew",	L":t"},
	{L":peers",		L":p"}
};
static std::unordered_map<std::wstring, std::function<void (void)>> commands=
{
	{L":q", [](void){running=false;}},
	{L":u", [](void){tapi2p::UI::Update();}},
	{L":d", [](void){tapi2p::UI::DelTab();}},
	{L":t", [](void){tapi2p::UI::AddTab(L"newtab");}},
	{L":p", [](void){tapi2p::UI::AddTab(tapi2p::UI::Peers);}}
};

std::wstring edatatowstr(evt_t* e)
{
	std::string edata(e->data, e->data_len);
	std::wstring wedata;
	wedata.resize(e->data_len);
	std::copy(edata.begin(), edata.end(), wedata.begin());
	return wedata;
}

static void handlelistpeers(evt_t* e, void* data)
{
	tapi2p::UI::Peers.Clear();
	tapi2p::UI::WriteLine(tapi2p::UI::Peers, edatatowstr(e));
}

static void handlemessage(evt_t* e, void* data)
{
	std::wstring nick;
	/*
	try
	{
		nick=tapi1p::UI::GetItem(e->addr, "Nick");
	}
	catch(const std::runtime_error&)
	{
		nick=tapi2p::UI::GetItem("Peers", e->addr);
	}*/
	std::string addr(e->addr);
	nick.resize(addr.length());
	std::copy(addr.begin(), addr.end(), nick.begin());
	tapi2p::UI::WriteLine(tapi2p::UI::Main(), L"[" + nick + L"] " + edatatowstr(e));
}

int corefd;
int main()
{
	if(!setlocale(LC_CTYPE, "en_US.UTF-8"))
	{
		std::cerr << "Cannot set UTF-8 encoding. Please make sure that en_US.UTF-8 encoding is installed." << std::endl;
	}
	corefd=core_socket();
	if(corefd==-1)
	{
		std::cerr << "tapi2p core not running!" << std::endl;
		return 1;
	}
	event_addlistener(ListPeers, handlelistpeers, NULL);
	event_addlistener(Message, handlemessage, NULL);
	eventsystem_start(corefd);
	tapi2p::UI::Init();
	while(running)
	{
		std::wstring i=tapi2p::UI::HandleInput();
		try
		{
			if(i.length()) commands[i]();
		} catch(const std::bad_function_call& e)
		{
			std::string s;
			s.resize(i.length());
			std::copy(i.begin(), i.end(), s.begin());
			event_send_simple(Message, (const unsigned char*)s.c_str(), s.length(), corefd);
			tapi2p::UI::WriteLine(tapi2p::UI::Main(), L"[" + tapi2p::UI::GetItem("Nick", "Account") + L"] " + i);
		}
	}
	tapi2p::UI::Destroy();
}
