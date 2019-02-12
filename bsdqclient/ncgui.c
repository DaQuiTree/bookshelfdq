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

static int get_choice(ui_page_type_e ptype, slider_t *sld, int logic_row_max);
static void show_shelf_info(int posx, int posy, shelf_entry_t *local_shelf);
static void move_slider(slider_t *sld);
static void print_error(char* error);
static void clear_line(int startx, int starty);

void ncgui_init(char* user)
{
    setlocale(LC_ALL, "");
    initscr();
    strcpy(login_user, user);
    mainwin = subwin(stdscr, WIN_HIGHT, WIN_WIDTH, 0, 0);
    box(mainwin, ACS_VLINE, ACS_HLINE);
    curs_set(0);
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
    mainmenu_slider.max_row = 3;

    move_slider(&mainmenu_slider);
    return((ui_menu_e)get_choice(page_mainmenu_e, &mainmenu_slider, 3));
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

    touchwin(stdscr);
    refresh();
}

void ncgui_display_lookthrough_page(void)
{
    int pad_posx = GREET_ROW+2, pad_posy = 8;
    int first_line = 0, max_rows, logic_rows, choice = 0;
    int i, nshelves = 0;
    shelf_entry_t local_shelf;
    slider_t slider;
    WINDOW *padwin;

    //标题
    mvwprintw(mainwin, TITLE_ROW, WIN_WIDTH/2-8, menu_option[menu_look_through_e]); 

    //分割竖线
    move(WIN_HIGHT/4, WIN_WIDTH/2);
    wvline(stdscr, ACS_VLINE, WIN_HIGHT/10*6);

    touchwin(stdscr);
    refresh();

    //pad填充内容
    padwin = newpad(PAD_HIGHT, PAD_WIDTH);
    for(i = 1; i <= MAX_SHELF_NUM; i++){
        if (clidb_shelf_exists(i)){
            if(!clidb_shelf_get(i, &local_shelf)){
                print_error(local_shelf.name);
                return;
            }
            mvwprintw(padwin, nshelves++, 0, local_shelf.name);
        }
    }

    //计算choice最大值
    if(nshelves > PAD_BOXED_HIGHT - 1)
        max_rows = PAD_BOXED_HIGHT - 1;
    else
        max_rows = nshelves - 1;
    logic_rows = nshelves - 1;

    //滑块初始位置
    slider.start_posx = pad_posx;
    slider.start_posy = pad_posy - 3;
    slider.nstep = 1;
    slider.current_row = slider.last_row = 0;
    slider.max_row = max_rows;

    move_slider(&slider);
    clidb_shelf_get(clidb_shelf_realno(first_line+choice+1), &local_shelf);
    show_shelf_info(pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, &local_shelf);

    while(choice != -1){
        prefresh(padwin, first_line, 0, pad_posx, pad_posy, pad_posx+PAD_BOXED_HIGHT-1, PAD_BOXED_WIDTH);
        choice = get_choice(page_lookthrough_e, &slider, logic_rows);
        if(choice != -1){
           //获取书架信息并显示
           clidb_shelf_get(clidb_shelf_realno(first_line+choice+1), &local_shelf);
           show_shelf_info(pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, &local_shelf);
           //滚动屏幕
           if(choice > max_rows)first_line++;
           if(choice == 0)first_line = 0;
        }
    }
}

static void show_shelf_info(int startx, int starty, shelf_entry_t *local_shelf)
{ 
    clear_line(startx, starty);
    mvwprintw(mainwin, startx, starty, "编号: %d", local_shelf->code);
    clear_line(startx+1, starty);
    mvwprintw(mainwin, startx+1, starty, "层数: %d", local_shelf->nfloors);
    clear_line(startx+2, starty);
    mvwprintw(mainwin, startx+2, starty, "打造时间: %s", local_shelf->building_time);

    touchwin(stdscr);
    refresh();
}

static void clear_line(int startx, int starty)
{
    char blank_line[WIN_WIDTH]={0};

    memset(blank_line, ' ', WIN_WIDTH-starty-1);
    mvwprintw(mainwin, startx, starty, blank_line);

    touchwin(stdscr);
    refresh();
}

static int get_choice(ui_page_type_e ptype, slider_t *sld, int logic_row)
{
    static int selected_row = 0;
    static ui_page_type_e local_ptype = page_nonsense_e;
    int key = 0;
    int local_key_esc = 27;//27:ESC

    if(local_ptype != ptype){
        local_ptype = ptype;
        selected_row = 0;
    }

    keypad(stdscr, TRUE);
    cbreak();
    noecho();

    while(key != KEY_ENTER && key != '\n' && key != local_key_esc){ 
        key = getch();
        if (key == KEY_UP){
            sld->last_row = selected_row;
            if(selected_row == 0)
                selected_row = logic_row;
            else
                selected_row--;
            sld->current_row = selected_row;
            move_slider(sld);
            if(ptype != page_mainmenu_e)break;
        }
        if(key == KEY_DOWN){
            sld->last_row = selected_row;
            if(selected_row == logic_row)
                selected_row = 0;
            else
                selected_row++;
            sld->current_row = selected_row;
            move_slider(sld);
            if(ptype != page_mainmenu_e)break;
        }
    }

    keypad(stdscr, FALSE);
    nocbreak();
    echo();

    if(key == local_key_esc)selected_row = -1;

    return(selected_row);
}

//移动并显示滑块
static void move_slider(slider_t *sld)
{
    if(sld->current_row > sld->max_row)sld->current_row = sld->max_row;
    if(sld->last_row > sld->max_row)sld->last_row = sld->max_row;

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

