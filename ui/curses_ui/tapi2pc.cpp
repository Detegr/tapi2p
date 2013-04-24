#include "curses_ui.h"
#include <functional>
#include <unordered_map>

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

int main()
{
	tapi2p::UI::Init();
	while(running)
	{
		std::wstring i=tapi2p::UI::HandleInput();
		try
		{
			commands[i]();
		} catch(const std::bad_function_call& e) {}
	}
}
