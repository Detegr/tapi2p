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
	int			UI::m_PeerWidth;
	const int	UI::m_InputHeight;
	int			UI::x;
	int			UI::y;
	bool		UI::Resized;
	C_Mutex		UI::m_Lock;

	void UI::Init()
	{
		m_PeerWidth=20;

		initscr();
		refresh();
		//noecho();
		keypad(stdscr,TRUE);
		cbreak();

		App=Window(COLS, LINES-m_InputHeight, 0, 0);
		App.SetBox();
		
		Content=Window(COLS-m_PeerWidth-3, LINES-m_InputHeight-2, 2, 1);
		scrollok(Content.Win(), TRUE);

		Peers=Window(m_PeerWidth, LINES-m_InputHeight, COLS-m_PeerWidth, 0);
		Peers.SetBox();

		PeerContent=Window(m_PeerWidth-4, LINES-m_InputHeight-2, COLS-m_PeerWidth+2, 1);

		mvprintw(LINES-m_InputHeight, 0, "%s", "tapi2p> ");

		refresh();
		Resized=false;
		x=COLS; y=LINES;
		Config& conf = PathManager::GetConfig();
		m_Lock.M_Lock();
		tapi2p::UI::Content.Write("Welcome to tapi2p, " + conf.Get("Account", "Nick"));
		m_Lock.M_Unlock();
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
		mvprintw(LINES-m_InputHeight, 0, "%s", "tapi2p> ");
		move(LINES-1, 8);
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
				PeerContent.Write(c.Get((*pt)->Sock_In.M_Ip().M_ToString(), "Nick"));
				oneway=false;
			}
			else if(oneway) PeerContent.Write(c.Get((*pt)->Sock_Out.M_Ip().M_ToString(), "Nick") + " [One-way]");
		}
		if(peers.empty()) tapi2p::UI::PeerContent.Write("");
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
}
