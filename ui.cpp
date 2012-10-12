#include <ncurses.h>
#include <string.h>

class Window
{
	private:
		int w;
		int h;
		int x;
		int y;
		WINDOW* win;
	public:
		Window(int w, int h, int x, int y) : w(w),h(h),x(x),y(y)
		{
			win=newwin(h,w,y,x);
			wrefresh(win);
		}
		void SetBox()
		{
			box(win, 0, 0);
			wrefresh(win);
		}
		~Window()
		{
			delwin(win);
		}
		WINDOW* Win() const
		{
			return win;
		}
};

int main()
{
	initscr();
	cbreak();
	char str[256];
	refresh();
	{
		Window winborder(COLS, LINES-1, 0, 0);
		winborder.SetBox();
		Window main(COLS-23,LINES-3,2,1);
		scrollok(main.Win(), 1);
		Window peerw(20, LINES-1, COLS-20, 0);
		peerw.SetBox();
		mvprintw(LINES-1, 0, "%s", "tapi2p> ");
		int y=0;
		do
		{
			memset(str, 0, 256);
			clrtoeol();
			getstr(str);
			if(y>getmaxy(main.Win())-1)
			{
				y=getmaxy(main.Win())-1;
				wscrl(main.Win(), 1);
			}
			mvwprintw(main.Win(), y, 0, str);
			int len=strlen(str);
			for(int z=len; z>0; z-=COLS-23) y++;
			//mvwaddstr(main.Win(), 2, 2, str);
			wrefresh(main.Win());
			move(LINES-1, 8);
		} while(strcmp(str, "q"));
	}
	endwin();
}
