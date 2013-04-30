#include "curses_ui.h"
#include <functional>
#include <unordered_map>
extern "C"
{
	#include "core.h"
	#include "event.h"
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
	tapi2p::UI::Init();
	while(running)
	{
		std::wstring i=tapi2p::UI::HandleInput();
		try
		{
			commands[i]();
		} catch(const std::bad_function_call& e)
		{
			std::string s;
			s.resize(i.length());
			std::copy(i.begin(), i.end(), s.begin());
			event_send_simple(Message, (const unsigned char*)s.c_str(), s.length(), corefd);
		}
	}
	tapi2p::UI::Destroy();
}
