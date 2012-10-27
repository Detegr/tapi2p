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
		refresh();
		noecho();
		keypad(stdscr,TRUE);
		cbreak();

		App=Window(COLS, LINES-m_InputHeight, 0, 0);
		App.SetBox();
		
		Content=Window(COLS-m_PeerWidth-3, LINES-m_InputHeight-2, 2, 1);
		scrollok(Content.Win(), TRUE);

		Peers=Window(m_PeerWidth, LINES-m_InputHeight, COLS-m_PeerWidth, 0);
		Peers.SetBox();

		PeerContent=Window(m_PeerWidth-4, LINES-m_InputHeight-2, COLS-m_PeerWidth+2, 1);
		Input=Window(COLS, m_InputHeight, 0, LINES-m_InputHeight);

		refresh();
		Resized=false;
		x=COLS; y=LINES;
		Config& conf = PathManager::GetConfig();
		m_Lock.M_Lock();
		tapi2p::UI::Content.Write(L"Welcome to tapi2p, " + conf.Getw("Account", "Nick"));
		m_Lock.M_Unlock();
		m_Prompt=L"tapi2p> ";
		move(LINES-1, m_PromptLen);
	}
	void UI::CheckSize()
	{
		if(x != COLS || y != LINES)
		{
			if(x <= m_PeerWidth)
			{
				m_PeerWidth=0;
			} else m_PeerWidth=20;
			UI::x=COLS;
			UI::y=LINES;
			wresize(App.Win(), UI::y-m_InputHeight, UI::x);
			wresize(Content.Win(), UI::y-m_InputHeight-2, UI::x-m_PeerWidth-3);
			wresize(Peers.Win(), UI::y-m_InputHeight, m_PeerWidth);
			wresize(PeerContent.Win(), UI::y-m_InputHeight-2, m_PeerWidth-4);
			mvwin(PeerContent.Win(), 1, UI::x-m_PeerWidth+2);
			mvwin(Peers.Win(), 0, UI::x-m_PeerWidth);
			App.Clear();
			Peers.Clear();
			App.SetBox();
			if(m_PeerWidth) Peers.SetBox();
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
			getyx(stdscr, y, x);
			get_wch(&ch);

			tapi2p::UI::Lock();
			tapi2p::UI::CheckSize();
			tapi2p::UI::Unlock();

			if(ch == L'\n') break;
			else if(ch == KEY_LEFT)
			{
				move(y,x-1);
				m_Cursor--;
			}
			else if(ch == KEY_RIGHT)
			{
				if(m_Cursor+1 <= m_StrLen)
				{
					move(y,x+1);
					m_Cursor++;
				}
				else continue;
			}
			else if(ch == KEY_END)
			{
				move(y, x+(m_StrLen-m_Cursor));
				m_Cursor=m_StrLen;
				continue;
			}
			else if(ch == KEY_HOME)
			{
				move(y, m_PromptLen);
				m_Cursor=0;
			}
			else if(ch == KEY_UP || ch==KEY_DOWN)
			{
			}
			else if(ch == KEY_BACKSPACE || ch==KEY_DC)
			{
				if(ch == KEY_BACKSPACE)
				{
					if(m_Cursor==0) continue;
					m_Cursor--;
				}
				if(m_StrLen>0)
				{
					memmove(&m_Str[m_Cursor], &m_Str[m_Cursor+1], m_StrLen*sizeof(wchar_t) - m_Cursor*sizeof(wchar_t) - 4);
					m_Str[m_StrLen-1]=' ';
					m_Lock.M_Lock();
					Input.Write(m_Prompt + m_Str);
					m_Lock.M_Unlock();
					m_Str[m_StrLen-1]=0;
					m_StrLen--;
					if(ch==KEY_BACKSPACE) move(y,x-1);
				}
				continue;
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
				move(y,x+1);
			}
			m_Lock.M_Lock();
			Input.Write(m_Prompt + m_Str);
			m_Lock.M_Unlock();
		}
		std::wstring cmd(m_Str);
		m_Cursor=0;
		move(0,m_PromptLen);
		werase(Input.Win());
		return cmd;
	}
}
