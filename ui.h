#pragma once
#include <string>
#include <list>
#include <vector>
#include <ncursesw/cursesw.h>
#include "dtglib/Concurrency.h"
using namespace dtglib;
namespace tapi2p
{
	class Window
	{
		private:
			int w;
			int h;
			int x;
			int y;
			int c;
			WINDOW* win;
		public:
			Window() : w(0),h(0),x(0),y(0),c(0),win(NULL) {}
			Window(int w, int h, int x, int y, bool activate=true) : w(w),h(h),x(x),y(y),c(0)
			{
				win=newwin(h,w,y,x);
				if(activate) wrefresh(win);
			}
			Window(const Window& rhs)
			{
				w=rhs.w;
				h=rhs.h;
				x=rhs.x;
				y=rhs.y;
				c=rhs.c;
				win=rhs.win;
			}
			Window& operator=(const Window& rhs)
			{
				if(&rhs != this)
				{
					w=rhs.w;
					h=rhs.h;
					x=rhs.x;
					y=rhs.y;
					c=rhs.c;
					win=rhs.win;
				}
				return *this;
			}
			void Write(const std::wstring& s, int x=0)
			{
				if(c>getmaxy(win)-1)
				{
					c=getmaxy(win)-1;
					wscrl(win,1);
				}
				mvwaddwstr(win, c, x, s.c_str());
				for(int i=s.size(); i>0; i-=w) c++;
			}
			void Clear()
			{
				werase(win);
				//wrefresh(win);
				c=0;
			}
			void Redraw()
			{
				redrawwin(win);
				wrefresh(win);
			}
			void SetBox()
			{
				box(win, 0, 0);
				wrefresh(win);
			}
			void Delete()
			{
				delwin(win);
				win=NULL;
			}
			WINDOW* Win() const
			{
				return win;
			}
	};

	struct TabWindow
	{
		Window w;
		std::wstring t;
		TabWindow(Window w, const std::wstring& t) : w(w), t(t) {}
	};

	class TabBar
	{
		friend class UI;
		private:
			int						m_Active;
			Window					m_Tabs;
			std::vector<TabWindow>	m_TabWindows;
			static const int		m_TabSpacing=2;
		public:
			TabBar() : m_Active(0) {}
			void Init(int w, int h);
			void Draw();
			Window& Active() { return m_TabWindows[m_Active].w; }
			void Add(const std::wstring& s, int w, int h, int x, int y)
			{
				m_TabWindows.push_back(TabWindow(Window(w,h,x,y,false), s));
				scrollok(m_TabWindows.back().w.Win(), TRUE);
			}
			void DeleteCurrent()
			{
				if(m_Active!=0)
				{
					std::vector<TabWindow>::iterator it = m_TabWindows.begin()+m_Active;
					TabWindow tw=*it;
					m_TabWindows.erase(it);
					m_Active--;
					tw.w.Delete();
				}
			}
			void Next()
			{
				m_Active = (m_Active+1) % m_TabWindows.size();
				m_TabWindows[m_Active].w.Redraw();
				wrefresh(m_TabWindows[m_Active].w.Win());
			}
			void Prev()
			{
				m_Active--;
				if(m_Active<0) m_Active=m_TabWindows.size()-1;
				m_TabWindows[m_Active].w.Redraw();
				wrefresh(m_TabWindows[m_Active].w.Win());
			}
			WINDOW* Win() const { return m_Tabs.Win(); }
			void Clear() { m_Tabs.Clear(); }
	};

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

		public:
			static int x;
			static int y;
			static bool Resized;
			static Window App;
			static Window Peers;
			static Window PeerContent;
			static Window Input;
			static TabBar Tabs;

			static void Init();
			static void Destroy();
			static void CheckSize();
			static void Lock();
			static void Unlock();
			static void Update();
			static void AddTab(const std::wstring& s);
			static void DelTab();
			static void NextTab();
			static void PrevTab();
			static void Write(Window& win, const std::wstring& s);
			static Window& Active() { return Tabs.Active(); }
			static Window& Main() { return Tabs.m_TabWindows[0].w; }
			static std::wstring HandleInput();
	};
}
