#include "curses_ui.h"
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


static void handlelistpeers(evt_t* e, void* data)
{
	std::string edata(e->data, e->data_len);
	std::wstring wedata;
	wedata.resize(e->data_len);
	std::copy(edata.begin(), edata.end(), wedata.begin());
	tapi2p::UI::Peers.Clear();
	tapi2p::UI::WriteLine(tapi2p::UI::Peers, wedata);
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
