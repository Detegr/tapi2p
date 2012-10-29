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
			int m_Width;
			int m_Height;
			int m_PosX;
			int m_PosY;
			int m_Cursor;
			WINDOW* m_Window;
		public:
			Window() : m_Width(0), m_Height(0), m_PosX(0), m_PosY(0), m_Cursor(0), m_Window(NULL) {}
			Window(int w, int h, int x, int y, bool activate=true) : m_Width(w), m_Height(h), m_PosX(x), m_PosY(y), m_Cursor(0)
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
			void Write(const std::wstring& s, int x=0)
			{
				if(m_Cursor>getmaxy(m_Window)-1)
				{
					m_Cursor=getmaxy(m_Window)-1;
					wscrl(m_Window,1);
				}
				mvwaddwstr(m_Window, m_Cursor, m_PosX, s.c_str());
				for(int i=s.size(); i>0; i-=m_Width) m_Cursor++;
			}
			void Clear()
			{
				werase(m_Window);
				m_Cursor=0;
			}
			void Redraw()
			{
				redrawwin(m_Window);
				wrefresh(m_Window);
			}
			void SetBox()
			{
				box(m_Window, 0, 0);
				wrefresh(m_Window);
			}
			void Delete()
			{
				delwin(m_Window);
				m_Window=NULL;
			}
			void Refresh()
			{
				wrefresh(m_Window);
			}
			WINDOW* Win() const
			{
				return m_Window;
			}
	};

	class TabWindow : public Window
	{
		private:
			bool			m_Static;
			std::wstring	m_Name;
		public:
			TabWindow() : m_Static(false) {}
			TabWindow(const std::wstring& name, int w, int h, int x, int y, bool activate=true) : m_Static(false), m_Name(name), Window(w,h,x,y,activate) {}
			const std::wstring& Name() const { return m_Name; }
			void SetStatic() { m_Static=true; }
			bool Static() const { return m_Static; }
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
			TabWindow& Active() { return m_TabWindows[m_Active]; }
			void Add(const std::wstring& s, int w, int h, int x, int y)
			{
				m_TabWindows.push_back(TabWindow(s,w,h,x,y,false));
				scrollok(m_TabWindows.back().Win(), TRUE);
			}
			void Add(const TabWindow& w)
			{
				m_TabWindows.push_back(w);
				scrollok(m_TabWindows.back().Win(), TRUE);
			}
			void DeleteCurrent()
			{
				if(m_Active!=0)
				{
					std::vector<TabWindow>::iterator it = m_TabWindows.begin()+m_Active;
					TabWindow tw=*it;
					m_TabWindows.erase(it);
					m_Active--;
					if(!tw.Static()) tw.Delete();
				}
			}
			void Next()
			{
				m_Active = (m_Active+1) % m_TabWindows.size();
				m_TabWindows[m_Active].Redraw();
				m_TabWindows[m_Active].Refresh();
			}
			void Prev()
			{
				m_Active--;
				if(m_Active<0) m_Active=m_TabWindows.size()-1;
				m_TabWindows[m_Active].Redraw();
				m_TabWindows[m_Active].Refresh();
			}
			void SetDirty(const TabWindow& w, bool dirty)
			{
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

		public:
			static int x;
			static int y;
			static bool Resized;
			static Window App;
			static Window Input;
			static TabWindow Peers;
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
			static Window& Main() { return Tabs.m_TabWindows[0]; }
			static std::wstring HandleInput();
	};
}
