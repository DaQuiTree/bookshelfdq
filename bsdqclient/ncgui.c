#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <locale.h>

#include "clisrv.h"
#include "clidb.h"
#include "ncgui.h"

//主屏幕尺寸
#define WIN_HIGHT 24
#define WIN_WIDTH 68

//pad尺寸
#define PAD_HIGHT 30
#define PAD_WIDTH WIN_WIDTH
#define PAD_BOXED_HIGHT 12
#define PAD_BOXED_WIDTH (WIN_WIDTH/10*5)

//各类信息行行号
#define TITLE_ROW 1
#define GREET_ROW 4
#define DETAILS_ROW 18 
#define PROMPT_ROW 22

char login_user[USER_NAME_LEN+1];

char *menu_option[] = {
    " 浏 览 书 架 ⚯  ",
    " 打 造 书 架 ⚒  ",
    " 检 索 图 书 ⚙  ",
    "    退 出 ⛷     ",
    0
};

WINDOW *mainwin;
WINDOW *padwin;

static int get_choice(slider_t *sld, int row_max);
static void move_slider(slider_t *sld);
static void print_error(char* error);

void ncgui_init(char* user)
{
    setlocale(LC_ALL, "");
    initscr();
    strcpy(login_user, user);
    mainwin = subwin(stdscr, WIN_HIGHT, WIN_WIDTH, 0, 0);
    box(mainwin, ACS_VLINE, ACS_HLINE);
    padwin = newpad(PAD_HIGHT, PAD_WIDTH);
}

void ncgui_close(void)
{
    delwin(mainwin);
    endwin();
}

void ncgui_clear_all_screen(void)
{
    clear();
    
    mainwin = subwin(stdscr, WIN_HIGHT, WIN_WIDTH, 0, 0);
    box(mainwin, ACS_VLINE, ACS_HLINE);
    move(TITLE_ROW+1, WIN_WIDTH/2-14);
    whline(stdscr, ACS_HLINE, 28);

    refresh();
}

ui_menu_e ncgui_get_choice(void)
{
    slider_t mainmenu_slider;

    mainmenu_slider.start_posx = GREET_ROW+2;
    mainmenu_slider.start_posy = WIN_WIDTH/2-10;
    mainmenu_slider.nstep = 2;
    mainmenu_slider.current_row = 0;
    mainmenu_slider.last_row = 0;

    return((ui_menu_e)get_choice(&mainmenu_slider, 3));
}

void ncgui_display_mainmenu_page(shelf_count_t sc, book_count_t bc)
{
    char greet[20] = "  欢迎: ";
    char details[80];
    char **option_ptr;
    int menu_row = GREET_ROW + 2;
    
    //标题
    mvwprintw(mainwin, TITLE_ROW, WIN_WIDTH/2-6, "书架管理系统");

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
    
    //显示菜单
    option_ptr = menu_option;
    while(*option_ptr){
        mvwprintw(mainwin, menu_row, WIN_WIDTH/2-8, *option_ptr);
        menu_row += 2;
        option_ptr++;
    }

    move(PROMPT_ROW, 1);
    touchwin(stdscr);
    refresh();
}

void ncgui_display_lookthrough_page(void)
{
    int pad_posx = GREET_ROW+2;
    int pad_posy = 4;
    shelf_entry_t local_shelf;
    int i,first_line = 0;

    //标题
    mvwprintw(mainwin, TITLE_ROW, WIN_WIDTH/2-8, menu_option[menu_look_through_e]); 

    //分割竖线
    move(WIN_HIGHT/4, WIN_WIDTH/2);
    wvline(stdscr, ACS_VLINE, WIN_HIGHT/10*6);

    touchwin(stdscr);
    refresh();

    //pad填充内容
    for(i = 1; i <= MAX_SHELF_NUM; i++){
        if (clidb_shelf_exists(i)){
            if(clidb_shelf_get(i, &local_shelf)){
                print_error("获取书架信息错误");
                return;
            }
            mvwprintw(padwin, i-1, 0, local_shelf.name);
        }
    }
    prefresh(padwin, first_line, 0, pad_posx, pad_posy, pad_posx+PAD_BOXED_WIDTH, pad_posy+PAD_BOXED_HIGHT);
}

static int get_choice(slider_t *sld, int row_max)
{
    static int selected_row = 0;
    int key = 0;

    move_slider(sld);

    keypad(stdscr, TRUE);
    cbreak();
    noecho();

    while(key != KEY_ENTER && key != '\n'){
        key = getch();
        if (key == KEY_UP){
            sld->last_row = selected_row;
            if(selected_row == 0)
                selected_row = row_max;
            else
                selected_row--;
            sld->current_row = selected_row;
            move_slider(sld);
        }
        if(key == KEY_DOWN){
            sld->last_row = selected_row;
            if(selected_row == row_max)
                selected_row = 0;
            else
                selected_row++;
            sld->current_row = selected_row;
            move_slider(sld);
        }
    }

    keypad(stdscr, FALSE);
    nocbreak();
    echo();

    return(selected_row);
}

//显示滑块
static void move_slider(slider_t *sld)
{
    //取消显示上一个滑块
    mvwprintw(mainwin, sld->start_posx + sld->last_row*sld->nstep, sld->start_posy, " ");

    //显示滑块
    mvwprintw(mainwin, sld->start_posx + sld->current_row*sld->nstep, sld->start_posy, "▊");

    touchwin(stdscr);
    refresh();
}

static void print_error(char* error)
{
    mvwprintw(mainwin, PROMPT_ROW, 1, error);
}

