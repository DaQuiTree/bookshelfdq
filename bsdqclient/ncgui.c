#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <locale.h>

#include "clisrv.h"

#define WIN_HIGHT 24
#define WIN_WIGHT 68

#define TITLE_ROW 1
#define GREET_ROW 3
#define DETAILS_ROW 18 
#define PROMPT_ROW 22

char login_user[USER_NAME_LEN+1];
char *menu_option[] = {
    "浏 览 书 架",
    "打 造 书 架",
    "检 索 图 书",
    0
};

WINDOW *mainwin;

void ncgui_init(char* user)
{
    setlocale(LC_ALL, "");
    initscr();
    strcpy(login_user, user);
    mainwin = subwin(stdscr, WIN_HIGHT, WIN_WIGHT, 0, 0);
    box(mainwin, ACS_VLINE, ACS_HLINE);
}

void ncgui_close(void)
{
    delwin(mainwin);
    endwin();
}

/*char ncgui_get_choice(void)*/
/*{*/
    /*int key = 0;*/
    /*int max_row = 3;*/
    /*int selected_row = 0;*/

    /*keypad(stdscr, TRUE);*/
    /*cbreak();*/
    /*noecho();*/

    /*while(key != 'q' && key != KEY_ENTER && key != '\n'){*/
        /*if (key == KEY_UP){*/
            /*if(selected_row == 0)*/
                /*selected_row = max_row - 1;*/
            /*else*/
                /*selected_row--;*/
        /*}*/
        /*if(key == KEY_DOWN){*/
            /*if(selected_row == (max_row -1))*/
                /*selected_row = 0;*/
            /*else*/
                /*selected_row++;*/
        /*}*/
        /*selected = *menu_option[selected_row];*/
        /*ncgui_draw_mainmenu(selected_row, */
    /*}*/
/*}*/

void ncgui_draw_mainmenu(int hl_row, shelf_count_t sc, book_count_t bc)
{
    char greet[20] = "  欢迎: ";
    char details[80];
    char **option_ptr;
    char current_row = 0;
    int menu_row = GREET_ROW + 2;
    
    //标题
    mvwprintw(mainwin, TITLE_ROW, WIN_WIGHT/2-6, "书架管理系统");
    move(TITLE_ROW+1, WIN_WIGHT/2-14);
    whline(stdscr, ACS_HLINE, 28);

    //欢迎行
    if(strlen(login_user) <= 8){
        sprintf(greet, "  欢迎: %s", login_user);
    }else{
        strncpy(greet+10, login_user, 8);
        strcpy(greet+18, "***");
    }
    mvwprintw(mainwin, GREET_ROW, 1, greet);

    //信息行
    sprintf(details, "  拥有: 书架%d个,图书%d册 在读%d册 借出%d册 未归档%d册",\
            sc.shelves_all, bc.books_all, bc.books_on_reading, bc.books_borrowed, bc.books_unsorted);
    mvwprintw(mainwin, DETAILS_ROW, 1, details);

    //显示menu
    option_ptr = menu_option;
    while(*option_ptr){
        if(current_row == hl_row) attron(A_STANDOUT);
        mvwprintw(mainwin,  menu_row, WIN_WIGHT/2-6, *option_ptr);
        if(current_row == hl_row) attroff(A_STANDOUT);
        menu_row += 2;
        current_row++;
        option_ptr++;
    }
    refresh();
}

