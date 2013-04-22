#pragma once
#include <string>
#include <list>
#include <vector>
#include <cursesw.h>
#include "dtglib/Concurrency.h"
#include <iostream>

using namespace dtglib;
namespace tapi2p
{
	class WindowBase
	{
		protected:
			WindowBase() : m_Width(0), m_Height(0), m_PosX(0), m_PosY(0), m_Window(NULL) {}
			WindowBase(int w, int h, int x, int y) :
				m_Width(w), m_Height(h), m_PosX(x), m_PosY(y), m_Window(NULL) {}

			int m_Width;
			int m_Height;
			int m_PosX;
			int m_PosY;
			WINDOW* m_Window;
		public:
			virtual void Clear()=0;
			virtual void Redraw()=0;
			virtual void Delete()=0;
			virtual void Refresh()=0;
			virtual WINDOW* Win() const
			{
				return m_Window;
			}
			virtual void Write(const std::wstring& s)=0;
			virtual void WriteLine(const std::wstring& s)=0;
			virtual void Resize(int x, int y)=0;
	};

	class Window : public WindowBase
	{
		private:
			static const int MAXMSG=100;
			int m_Cursor;
		public:
			std::vector<std::wstring> m_Messages;
			Window() : WindowBase(), m_Cursor(0) {}
			Window(int w, int h, int x, int y, bool activate=true) :
				WindowBase(w,h,x,y), m_Cursor(0)
			{
				m_Window=newwin(h,w,y,x);
				if(activate) Refresh();
			}
			Window(const Window& rhs)
			{
				m_Width=rhs.m_Width;
				m_Height=rhs.m_Height;
				m_PosX=rhs.m_PosX;
				m_PosY=rhs.m_PosY;
				m_Cursor=rhs.m_Cursor;
				m_Window=rhs.m_Window;
			}
			Window& operator=(const Window& rhs)
			{
				if(&rhs != this)
				{
					m_Width=rhs.m_Width;
					m_Height=rhs.m_Height;
					m_PosX=rhs.m_PosX;
					m_PosY=rhs.m_PosY;
					m_Cursor=rhs.m_Cursor;
					m_Window=rhs.m_Window;
				}
				return *this;
			}

			virtual void Write(const std::wstring& s)
			{
				mvwaddwstr(m_Window, 0, 0, s.c_str());
			}

			virtual void WriteLine(const std::wstring& s)
			{
				if(m_Cursor>getmaxy(m_Window)-1)
				{
					m_Cursor=getmaxy(m_Window)-1;
					wscrl(m_Window,1);
				}
				std::wstring str=s;
				str+=L"\n";
				m_Messages.push_back(str);
				if(m_Messages.size() > MAXMSG) m_Messages.erase(m_Messages.begin());
				waddwstr(m_Window, str.c_str());
			}
			void WriteMsg(const std::wstring& s)
			{
				if(m_Cursor>getmaxy(m_Window)-1)
				{
					m_Cursor=getmaxy(m_Window)-1;
					wscrl(m_Window,1);
				}
				waddwstr(m_Window, s.c_str());
			}
			virtual void Clear()
			{
				werase(m_Window);
				m_Cursor=0;
			}
			virtual void Redraw()
			{
				redrawwin(m_Window);
				wrefresh(m_Window);
			}
			virtual void Delete()
			{
				delwin(m_Window);
				m_Window=NULL;
			}
			virtual void Refresh()
			{
				wrefresh(m_Window);
			}
			virtual void Resize(int x, int y)
			{
				wresize(m_Window, y, x);
			}
	};

	class Pad : public WindowBase
	{
		private:
			int m_CursorX;
			int m_CursorY;
		public:
			Pad() {}
			Pad(int w, int h, int w2, int h2, int x, int y) :
				WindowBase(w2,h2,x,y), m_CursorX(0), m_CursorY(0)
			{
				m_Window=newpad(h,w);
			}
			Pad(const Pad& rhs)
			{
				m_CursorX=rhs.m_CursorX;
				m_CursorY=rhs.m_CursorY;
				m_Width=rhs.m_Width;
				m_Height=rhs.m_Height;
				m_PosX=rhs.m_PosX;
				m_PosY=rhs.m_PosY;
				m_Window=rhs.m_Window;
			}
			Pad& operator=(const Pad& rhs)
			{
				if(this!=&rhs)
				{
					m_CursorX=rhs.m_CursorX;
					m_CursorY=rhs.m_CursorY;
					m_Width=rhs.m_Width;
					m_Height=rhs.m_Height;
					m_PosX=rhs.m_PosX;
					m_PosY=rhs.m_PosY;
					m_Window=rhs.m_Window;
				}
			}
			virtual void Clear()
			{
				werase(m_Window);
			}
			virtual void Redraw()
			{
				redrawwin(m_Window);
				Refresh();
			}
			virtual void Refresh()
			{
				prefresh(m_Window, m_CursorY, m_CursorX, m_PosY, m_PosX, m_PosY+m_Height, m_Width-1);
			}
			virtual void Delete()
			{
				delwin(m_Window);
				m_Window=NULL;
			}
			virtual void Write(const std::wstring& s)
			{
				mvwaddwstr(m_Window, 0, 0, s.c_str());
			}
			virtual void WriteLine(const std::wstring& s)
			{
				mvwaddwstr(m_Window, 0, 0, s.c_str());
			}
			void Move(int amountx, int amounty)
			{
				m_PosX+=amountx;
				m_PosY+=amounty;
			}
			void Resize(int w, int h, int x, int y)
			{
				m_Width=w;
				m_Height=h;
				m_PosX=x;
				m_PosY=y;
				Refresh();
			}
			virtual void Resize(int x, int y)
			{
			}
	};

	class TabWindow : public Window
	{
		friend class TabBar;
		private:
			bool			m_Added;
			bool			m_Static;
			bool			m_Dirty;
			std::wstring	m_Name;
		public:
			TabWindow() : m_Added(false), m_Static(false), m_Dirty(false) {}
			TabWindow(const std::wstring& name, int w, int h, int x, int y, bool activate=true) : m_Added(false), m_Static(false), m_Dirty(false), m_Name(name), Window(w,h,x,y,activate) {}
			const std::wstring& Name() const { return m_Name; }
			void SetStatic() { m_Static=true; }
			bool Static() const { return m_Static; }
			bool Dirty() const { return m_Dirty; }
			bool Added() const { return m_Added; }
			void SetDirty(bool dirty)
			{
				m_Dirty=dirty;
			}
	};

	class TabBar
	{
		friend class UI;
		private:
			int						m_Active;
			Window					m_Tabs;
			std::vector<TabWindow*>	m_TabWindows;
			static const int		m_TabSpacing=2;
		public:
			TabBar() : m_Active(0) {}
			void Delete()
			{
				for(std::vector<TabWindow*>::iterator it=m_TabWindows.begin(); it!=m_TabWindows.end(); ++it)
				{
					bool s=(*it)->Static();
					(*it)->Delete();
					if(!s) delete *it;
				}
			}
			void Init(int w, int h);
			void Draw();
			TabWindow& Active() { return *m_TabWindows[m_Active]; }
			void Add(const std::wstring& s, int w, int h, int x, int y)
			{
				m_TabWindows.push_back(new TabWindow(s,w,h,x,y,false));
				scrollok(m_TabWindows.back()->Win(), TRUE);
				m_TabWindows.back()->m_Added=true;
			}
			void Add(TabWindow& w)
			{
				m_TabWindows.push_back(&w);
				scrollok(m_TabWindows.back()->Win(), TRUE);
				m_TabWindows.back()->m_Added=true;
			}
			void DeleteCurrent()
			{
				if(m_Active!=0)
				{
					std::vector<TabWindow*>::iterator it = m_TabWindows.begin()+m_Active;
					TabWindow* tw=*it;
					m_TabWindows.erase(it);
					m_Active--;
					tw->m_Added=false;
					if(!tw->Static())
					{
						tw->Delete();
						delete tw;
					}
				}
			}
			void Next()
			{
				m_Active = (m_Active+1) % m_TabWindows.size();
				m_TabWindows[m_Active]->Redraw();
				m_TabWindows[m_Active]->Refresh();
				m_TabWindows[m_Active]->SetDirty(false);
			}
			void Prev()
			{
				m_Active--;
				if(m_Active<0) m_Active=m_TabWindows.size()-1;
				m_TabWindows[m_Active]->Redraw();
				m_TabWindows[m_Active]->Refresh();
				m_TabWindows[m_Active]->SetDirty(false);
			}
			WINDOW* Win() const { return m_Tabs.Win(); }
			void Clear() { m_Tabs.Clear(); }
	};
/*
	class PeerWindow : public TabWindow
	{
	};
*/
	class UI
	{
		private:
			static const int	m_InputHeight=1;
			static const int	m_StringMax=256;
			static int			m_PeerWidth;
			static C_Mutex		m_Lock;
			static wchar_t		m_Str[m_StringMax];
			static int 			m_StrLen;
			static int			m_Cursor;
			static std::wstring m_Prompt;
			static const int	m_PromptLen=8;
			static void Write(WindowBase& win, const std::wstring& s, bool line);

		public:
			static int x;
			static int y;
			static Window App;
			static Window Prompt;
			static Pad Input;
			static TabWindow Peers;
			static TabBar Tabs;

			static void Init();
			static void Destroy();
			static void CheckSize();
			static void Lock();
			static void Unlock();
			static void Update();
			static void AddTab(const std::wstring& s);
			static void AddTab(TabWindow& tw);
			static void DelTab();
			static void NextTab();
			static void PrevTab();
			static void Write(WindowBase& win, const std::wstring& s);
			static void WriteLine(Window& win, const std::wstring& s);
			static void WriteLine(TabWindow& win, const std::wstring& s);
			static TabWindow& Active() { return Tabs.Active(); }
			static TabWindow& Main() { return *Tabs.m_TabWindows[0]; }
			static std::wstring HandleInput();
	};
}
