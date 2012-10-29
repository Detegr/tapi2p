#pragma once
#include <string>
#include <list>
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
			bool copied;
		public:
			Window() : w(0),h(0),x(0),y(0),c(0),win(NULL) {}
			Window(int w, int h, int x, int y) : w(w),h(h),x(x),y(y),c(0)
			{
				win=newwin(h,w,y,x);
				wrefresh(win);
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
				wrefresh(win);
			}
			void Clear()
			{
				wclear(win);
				wrefresh(win);
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
			}
			WINDOW* Win() const
			{
				return win;
			}
	};

	class TabBar
	{
		friend class UI;
		private:
			int						m_Active;
			Window					m_Tabs;
			std::list<std::wstring>	m_TabTexts;
			static const int		m_TabSpacing=2;
		public:
			TabBar() : m_Active(0) {}
			void Init(int w, int h);
			void Draw();
			void Add(const std::wstring& s)
			{
				m_TabTexts.push_back(s);
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
			static Window Content;
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
			static void Write(Window& win, const std::wstring& s);
			static std::wstring HandleInput();
	};
}
