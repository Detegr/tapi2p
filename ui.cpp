#include "ui.h"
#include <string.h>
#include <signal.h>
#include "PeerManager.h"
#include "PathManager.h"
#include "Config.h"

namespace tapi2p
{
	Window		UI::App;
	Window		UI::Content;
	Window		UI::Peers;
	Window		UI::PeerContent;
	Window		UI::Input;
	int			UI::m_PeerWidth;
	const int	UI::m_InputHeight;
	int			UI::x;
	int			UI::y;
	bool		UI::Resized;
	C_Mutex		UI::m_Lock;
	int			UI::m_Cursor;
	const int	UI::m_StringMax;
	wchar_t		UI::m_Str[m_StringMax];
	int			UI::m_StrLen;
	std::wstring UI::m_Prompt;
	const int	UI::m_PromptLen;

	void UI::Init()
	{
		m_PeerWidth=20;

		initscr();
		noecho();
		cbreak();

		App=Window(COLS, LINES-m_InputHeight, 0, 0);
		App.SetBox();
		
		Content=Window(COLS-m_PeerWidth-3, LINES-m_InputHeight-2, 2, 1);
		scrollok(Content.Win(), TRUE);

		Peers=Window(m_PeerWidth, LINES-m_InputHeight, COLS-m_PeerWidth, 0);
		Peers.SetBox();

		PeerContent=Window(m_PeerWidth-4, LINES-m_InputHeight-2, COLS-m_PeerWidth+2, 1);
		Input=Window(COLS, m_InputHeight, 0, LINES-m_InputHeight);
		keypad(Input.Win(),TRUE);
		keypad(stdscr,TRUE);

		Resized=false;
		x=COLS; y=LINES;
		Config& conf = PathManager::GetConfig();
		Write(Content, L"Welcome to tapi2p, " + conf.Getw("Account", "Nick"));
		m_Prompt=L"tapi2p> ";
		wmove(Input.Win(), 0, m_PromptLen);
	}
	void UI::CheckSize()
	{
		if(x != COLS || y != LINES)
		{
			UI::x=COLS;
			UI::y=LINES;
			wresize(App.Win(), UI::y-m_InputHeight, UI::x);
			wresize(Input.Win(), m_InputHeight, UI::x);
			wresize(Content.Win(), UI::y-m_InputHeight-2, UI::x-m_PeerWidth-3);
			wresize(Peers.Win(), UI::y-m_InputHeight, m_PeerWidth);
			wresize(PeerContent.Win(), UI::y-m_InputHeight-2, m_PeerWidth-4);
			mvwin(PeerContent.Win(), 1, UI::x-m_PeerWidth+2);
			mvwin(Peers.Win(), 0, UI::x-m_PeerWidth);
			mvwin(Input.Win(), UI::y-m_InputHeight, 0);
			App.Clear();
			Peers.Clear();
			App.SetBox();
			Peers.SetBox();
			Content.Redraw();
			PeerContent.Redraw();
		}
	}

	void UI::Update()
	{
		Config& c = PathManager::GetConfig();
		m_Lock.M_Lock();
		PeerContent.Clear();
		std::vector<Peer*> peers=PeerManager::Do();
		for(std::vector<Peer*>::const_iterator pt=peers.begin(); pt!=peers.end(); ++pt)
		{
			bool oneway=true;
			if((*pt)->m_Connectable && (*pt)->Sock_In.M_Fd()>0)
			{
				PeerContent.Write(c.Getw((*pt)->Sock_In.M_Ip().M_ToString(), "Nick"));
				oneway=false;
			}
			else if(oneway) PeerContent.Write(c.Getw((*pt)->Sock_Out.M_Ip().M_ToString(), "Nick") + L" [One-way]");
		}
		if(peers.empty()) tapi2p::UI::PeerContent.Write(L"");
		PeerManager::Done();
		m_Lock.M_Unlock();
	}

	void UI::Destroy()
	{
		App.Delete();
		Content.Delete();
		Peers.Delete();
		endwin();
	}

	void UI::Lock()
	{
		m_Lock.M_Lock();
	}
	void UI::Unlock()
	{
		m_Lock.M_Unlock();
	}

	std::wstring UI::HandleInput()
	{
		wint_t ch=0;
		memset(m_Str, 0, m_StringMax-1);
		m_Lock.M_Lock();
		Input.Write(m_Prompt);
		m_Lock.M_Unlock();
		bool multibyte=false;
		m_StrLen=0;
		
		while(1)
		{
			int x, y;
			getyx(Input.Win(), y, x);
			wget_wch(Input.Win(), &ch);
/*
tapi2p::UI::Lock();
std::wstringstream ss;
std::wstring s;
ss << ch;
ss >> s;
tapi2p::UI::Write(Content, s);
tapi2p::UI::Unlock();
*/

			if(ch == L'\n') break;
			else if(ch == KEY_LEFT)
			{
				wmove(Input.Win(), y,x-1);
				m_Cursor--;
			}
			else if(ch == KEY_RIGHT)
			{
				if(m_Cursor+1 <= m_StrLen)
				{
					wmove(Input.Win(), y,x+1);
					m_Cursor++;
				}
				else continue;
			}
			else if(ch == KEY_END)
			{
				wmove(Input.Win(), y, x+(m_StrLen-m_Cursor));
				m_Cursor=m_StrLen;
				continue;
			}
			else if(ch == KEY_HOME)
			{
				wmove(Input.Win(), y, m_PromptLen);
				m_Cursor=0;
			}
			else if(ch == KEY_UP || ch==KEY_DOWN)
			{
			}
			else if(ch == KEY_BACKSPACE || ch == 127 || ch==KEY_DC)
			{
				if(ch == KEY_BACKSPACE || ch == 127)
				{
					if(m_Cursor==0) continue;
					m_Cursor--;
				}
				else if(m_Cursor == m_StrLen && ch == KEY_DC) continue;
				if(m_StrLen>0)
				{
					memmove(&m_Str[m_Cursor], &m_Str[m_Cursor+1], m_StrLen*sizeof(wchar_t) - m_Cursor*sizeof(wchar_t) - 4);
					m_Str[m_StrLen-1]=' ';
					Write(Input, m_Prompt + m_Str);
					m_Str[m_StrLen-1]=0;
					m_StrLen--;
					if(ch==KEY_BACKSPACE || ch == 127) wmove(Input.Win(), y,x-1);
				}
				continue;
			}
			else if(ch == KEY_RESIZE)
			{
				tapi2p::UI::Lock();
				tapi2p::UI::CheckSize();
				tapi2p::UI::Unlock();
			}
			else if(m_Cursor<m_StringMax-1)
			{
				if(m_Str[m_Cursor]==0) // We haven't gone back
				{
					m_Str[m_Cursor++]=ch;
					m_StrLen++;
				}
				else if(m_StrLen<m_StringMax-1)
				{
					memmove(&m_Str[m_Cursor+1], &m_Str[m_Cursor], m_StrLen*sizeof(wchar_t)-m_Cursor*sizeof(wchar_t));
					m_Str[m_Cursor++]=ch;
					m_StrLen++;
				}
				else continue;
				wmove(Input.Win(), 0, x+1);
			}
			m_Lock.M_Lock();
			Write(Input, m_Prompt + m_Str);
			m_Lock.M_Unlock();
		}
		m_Cursor=0;
		//wmove(Input.Win(), 0,m_PromptLen);
		werase(Input.Win());
		return m_Str;
	}
	
	void UI::Write(Window& win, const std::wstring& s)
	{
		int ro,co;
		m_Lock.M_Lock();
		getyx(Input.Win(),ro,co);
		win.Write(s);
		wmove(Input.Win(), 0, co);
		wrefresh(Input.Win());
		m_Lock.M_Unlock();
	}
}
