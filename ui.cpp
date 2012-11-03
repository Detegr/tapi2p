#include "ui.h"
#include <string.h>
#include <signal.h>
#include "PeerManager.h"
#include "PathManager.h"
#include "Config.h"

namespace tapi2p
{
	Window		UI::App;
	TabWindow	UI::Peers;
	Window		UI::Input;
	TabBar		UI::Tabs;
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

	void TabBar::Init(int w, int h)
	{
		m_Tabs=Window(w, h, 0, 0);
		UI::AddTab(L"Chat");
	}

	void TabBar::Draw()
	{
		tapi2p::UI::Lock();
		waddch(m_Tabs.Win(), ACS_LTEE);
		int j=1;
		int z=0;
		for(std::vector<TabWindow*>::iterator it=m_TabWindows.begin(); it!=m_TabWindows.end(); ++it, ++z)
		{
			for(int i=0; i<m_TabSpacing; ++i, ++j) waddch(m_Tabs.Win(), ACS_HLINE);
			waddch(m_Tabs.Win(), ACS_RTEE);
			if(z==m_Active) wattron(m_Tabs.Win(), A_UNDERLINE);
			if((*it)->Dirty()) wattron(m_Tabs.Win(), A_STANDOUT);
			waddwstr(m_Tabs.Win(), (*it)->Name().c_str());
			wattroff(m_Tabs.Win(), A_UNDERLINE);
			wattroff(m_Tabs.Win(), A_STANDOUT);
			waddch(m_Tabs.Win(), ACS_LTEE);
			j += 2 + (*it)->Name().length();
		}
		for(int k=0; k<(UI::x-j); ++k) waddch(m_Tabs.Win(), ACS_HLINE);
		m_Tabs.Refresh();
		tapi2p::UI::Unlock();
	}

	void UI::Init()
	{
		m_PeerWidth=20;

		initscr();
		noecho();
		cbreak();

		App=Window(COLS, LINES-m_InputHeight, 0, 0);
		x=COLS; y=LINES;
		//App.SetBox();
		wborder(App.Win(), ' ', ' ', 'c', ACS_HLINE, 'e', ACS_RTEE, ACS_HLINE, ACS_HLINE);
		App.Refresh();
		

		Tabs.Init(x-1, 1);
		Peers=TabWindow(L"Peers", x-3, y-2-m_InputHeight, 1, 1);
		Peers.SetStatic();
		Tabs.Add(Peers);
		
		Input=Window(COLS, m_InputHeight, 0, LINES-m_InputHeight);
		Tabs.Clear();
		Tabs.Draw();
		keypad(Input.Win(),TRUE);
		keypad(stdscr,TRUE);

		Resized=false;
		Config& conf = PathManager::GetConfig();
		WriteLine(Main(), L"Welcome to tapi2p, " + conf.Getw("Account", "Nick"));
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
			wresize(Tabs.Win(), 1, UI::x-1);
			wresize(Input.Win(), m_InputHeight, UI::x);
			for(std::vector<TabWindow*>::iterator it=Tabs.m_TabWindows.begin(); it!=Tabs.m_TabWindows.end(); ++it)
			{
				wresize((*it)->Win(), UI::y-m_InputHeight-2, UI::x-3);
			}
			mvwin(Input.Win(), UI::y-m_InputHeight, 0);
			App.Clear();
			wborder(App.Win(), ' ', ' ', 'c', ACS_HLINE, 'e', ACS_RTEE, ACS_HLINE, ACS_HLINE);
			App.Refresh();
			Tabs.Clear();
			Tabs.Draw();
			Active().Clear();
			for(std::vector<std::wstring>::const_iterator it=Active().m_Messages.begin(); it!=Active().m_Messages.end(); ++it)
			{
				Active().WriteMsg(*it);
			}
			Active().Refresh();
		}
	}

	void UI::Update()
	{
		Config& c = PathManager::GetConfig();
		m_Lock.M_Lock();
		Peers.Clear();
		std::vector<Peer*> peers=PeerManager::Do();
		for(std::vector<Peer*>::const_iterator pt=peers.begin(); pt!=peers.end(); ++pt)
		{
			bool oneway=true;
			if((*pt)->m_Connectable && (*pt)->Sock_In.M_Fd()>0)
			{
				WriteLine(Peers, c.Getw((*pt)->Sock_In.M_Ip().M_ToString(), "Nick"));
				oneway=false;
			}
			else if(oneway) WriteLine(Peers, c.Getw((*pt)->Sock_Out.M_Ip().M_ToString(), "Nick") + L" [One-way]");
		}
		if(peers.empty()) tapi2p::UI::WriteLine(Peers, L"");
		PeerManager::Done();
		m_Lock.M_Unlock();
	}

	void UI::Destroy()
	{
		Peers.Delete();
		Tabs.Delete();
		App.Delete();
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
		memset(m_Str, 0, (m_StringMax-1)*sizeof(wchar_t));
		m_Lock.M_Lock();
		Input.Write(m_Prompt);
		m_Lock.M_Unlock();
		m_StrLen=0;
		m_Cursor=0;
		WriteLine(Main(), L"Reset");
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
tapi2p::UI::Write(Active(), s);
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
			else if(ch == 9) // tab
			{
				tapi2p::UI::NextTab();
			}
			else if(ch == 353) // shift-tab
			{
				tapi2p::UI::PrevTab();
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
			if((m_StrLen + m_PromptLen) > UI::x-1)
			{
				Write(Input, m_Prompt + &m_Str[m_StrLen - (UI::x - m_PromptLen - 1)]);
			}
			else Write(Input, m_Prompt + m_Str);
			m_Lock.M_Unlock();
		}
		m_Cursor=0;
		//wmove(Input.Win(), 0,m_PromptLen);
		werase(Input.Win());
		return m_Str;
	}
	
	void UI::Write(Window& win, const std::wstring& s, bool line)
	{
		int ro,co;
		m_Lock.M_Lock();
		getyx(Input.Win(),ro,co);
		if(line) win.WriteLine(s);
		else win.Write(s);
		if(Active().Win() == win.Win()) wrefresh(win.Win());
		wmove(Input.Win(), 0, co);
		Input.Refresh();
		m_Lock.M_Unlock();
	}
	void UI::Write(Window& win, const std::wstring& s)
	{
		Write(win,s,false);
	}

	void UI::WriteLine(Window& win, const std::wstring& s)
	{
		Write(win,s,true);
	} 

	void UI::WriteLine(TabWindow& win, const std::wstring& s)
	{
		Write((Window&)win,s, true);
		if(Active().Win() != win.Win())
		{
			win.SetDirty(true);
			Tabs.Clear();
			Tabs.Draw();
		}
	}
	void UI::AddTab(const std::wstring& s)
	{
		m_Lock.M_Lock();
		Tabs.Add(s, x-2, y-2-m_InputHeight, 1, 1);
		Tabs.Clear();
		m_Lock.M_Unlock();
		Tabs.Draw();
	}
	void UI::AddTab(TabWindow& tw)
	{
		if(!tw.Added())
		{
			m_Lock.M_Lock();
			Tabs.Add(tw);
			Tabs.Clear();
			m_Lock.M_Unlock();
			Tabs.Draw();
		}
	}
	void UI::DelTab()
	{
		m_Lock.M_Lock();
		Tabs.DeleteCurrent();
		Tabs.Clear();
		Active().Redraw();
		Active().Refresh();
		m_Lock.M_Unlock();
		Tabs.Draw();
	}
	void UI::NextTab()
	{
		m_Lock.M_Lock();
		Tabs.Next();
		Tabs.Clear();
		m_Lock.M_Unlock();
		Tabs.Draw();
	}
	void UI::PrevTab()
	{
		m_Lock.M_Lock();
		Tabs.Prev();
		Tabs.Clear();
		m_Lock.M_Unlock();
		Tabs.Draw();
	}
}
