#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <locale.h>

#include "ncgui.h"
#include "clisrv.h"
#include "clishell.h"
#include "socketcom.h"
#include "clidb.h"

char HEX_ASC[] = {'0', '1', '2', '3',\
                  '4', '5', '6', '7',\
                  '8', '9', 'A', 'B',\
                  'C', 'D', 'E', 'F'};


//主屏幕尺寸
#define WIN_HIGHT 24
#define WIN_WIDTH 68

//pad尺寸
#define PAD_HIGHT DEFAULT_FINDS
#define PAD_WIDTH WIN_WIDTH
#define PAD_BOXED_HIGHT 12
#define PAD_BOXED_WIDTH (WIN_WIDTH/10*5)

//各类信息行行号
#define TITLE_ROW 1
#define GREET_ROW 4
#define DETAILS_ROW 18 
#define Q_ROW 20
#define PROMPT_ROW 21

//按键定义
#define LOCAL_KEY_NON_SENSE 0
#define LOCAL_KEY_CERTAIN_MEANING 1

char login_user[USER_NAME_LEN+1];

char *menu_option[] = {
    " ⚯  浏 览 书 架  ",
    " ⚒  打 造 书 架  ",
    " ⚙  检 索 图 书  ",
    0
};

char *newbook_option[] = {
    "书 名:",
    "作 者:",
    "层 数:",
    "打 签:",
    "完 成 ",
    0
};

char *lookthrough_option[] = {
    " 打  开 ",
    " 上  新 ",
    " 图  示 ",
    " 拆  除 ",
    0,
    " 打  开 ",
    " 上  新 ",
    " 图  示 ",
    " 拆  除 ",
    " 拾  遗 ",
    0
};

char *bookinfo_option[] = {
    " 阅  读 ",
    " 外  借 ",
    " 打标签 ",
    " 下  架 ",
    0,
    " 还  书 ",
    " 打标签 ",
    " 下  架 ",
    0
};

char *shelfdel_option[] = {
    " 招  领 ",
    " 弃  置 ",
    0
};

char *searchbook_option[] ={
    " 锁定书架 ",
    " 开始检索 ",
    0
};

book_count_t gui_bc;
shelf_count_t gui_sc;
WINDOW *mainwin;
int cursor_state = 0;

//通用函数
static int get_choice(ui_page_type_e ptype, slider_t *sld, int logic_row_max);
static void scroll_pad(slider_t *sld, int logic_row, int *nline);
static void move_slider(slider_t *sld, int unset);
static void error_line(char* error);
static void clear_line(WINDOW *win, int startx, int starty, int nlines, int line_width);
static int get_confirm(const char *prompt, char *option_default);
static char *string_filter(char *fstr, char ch_sub);
static int string_splitter(char *des_str, int des_len, char *src_str, int part_pos, char ch_aim);

//浏览书架相关
static ui_menu_e display_lookthrough_page(int select_mode);
static int pad_fill_shelves_info(WINDOW **win, slider_t *sld, int *para_logic_row);
static void show_shelf_info(int posx, int posy, shelf_entry_t *user_shelf, book_count_t *user_bc);
static void draw_lt_option_box(WINDOW **win, slider_t *sld, char *option[]);
static void destroy_lt_option_box(WINDOW **win);
static int display_shelfprofile_page(shelf_entry_t *user_shelf);

//浏览书籍相关
static int display_bookinfo_page(shelf_entry_t *user_shelf, int collect_mode, book_entry_t *search_entry);
static int next_page_bookinfo(WINDOW *win, int *shelfno, int *bookno, book_entry_t *search_entry);
static void show_book_info(int startx, int starty, book_entry_t *user_book, int bShowShelf);
static void split_labels(char *label, char rev[][37]);
static int draw_bi_tagging_box(book_entry_t *user_book, int posx, int posy);

//上架新书相关
static int add_newbook_page(shelf_entry_t *user_shelf);
static void get_newbook_info(book_entry_t *user_book);
static void select_newbook_floor(shelf_entry_t *user_shelf, book_entry_t *user_book, int posx, int posy);
static void show_newbook_info(book_entry_t *user_book);

//删除书架相关
static int delete_shelf_page(shelf_entry_t *user_shelf, book_count_t *sc);

//检索图书相关
void draw_searchbook_page_framework(WINDOW *win, int shelfno);
static void print_searchbook_prompt(void);
static int handle_searchbook_keyword(char *keyword, book_entry_t *user_book);
static void show_searchbox_info(book_entry_t *local_book);

//打造书架相关
static int check_shelf_name(char *name);
static void draw_shelf_profile(WINDOW *win, int nfloors, unsigned char depthArr[]);
static void set_shelf_parameter(WINDOW *win, int nfloors, unsigned char depthArr[]);

int ncgui_connect_to_server(char* hostname, char *user)
{
    strcpy(login_user, user);

    if(!client_initialize(hostname, login_user)){
        fprintf(stderr, "client initialize failed\n");
        return(0);
    }
    return(1);
}

int ncgui_sync_from_server(void)
{
    if(!client_shelves_info_sync(&gui_sc)){
        fprintf(stderr, "get client shelve info failed\n");
        if(mainwin != NULL)error_line("服务器同步数据异常");
        return(0);
    }
    if(!client_books_count_sync(&gui_bc)){
        fprintf(stderr, "get client books info failed\n");
        if(mainwin != NULL)error_line("服务器同步数据异常");
        return(0);
    }

    return(1);
}

void ncgui_init(void)
{
    setlocale(LC_ALL, "");
    initscr();
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
    int op_key;

    mainmenu_slider.win = stdscr;
    mainmenu_slider.start_posx = GREET_ROW+4;
    mainmenu_slider.start_posy = WIN_WIDTH/2-10;
    mainmenu_slider.nstep = 3;
    mainmenu_slider.current_row = 0;
    mainmenu_slider.last_row = 0;
    mainmenu_slider.max_row = 2;

    move_slider(&mainmenu_slider, 0);
    op_key = get_choice(page_mainmenu_e, &mainmenu_slider, 2);
    
    if(op_key == '\n' || op_key == KEY_ENTER){
        return((ui_menu_e)mainmenu_slider.current_row);
    }else if(op_key == KEY_BACKSPACE){
        if(get_confirm("即将退出程序,确定?", "No"))
            return(menu_quit_e);
    }

    return(menu_non_sense_e);
}

void ncgui_display_mainmenu_page(void)
{
    char greet[20] = "  欢迎: ";
    char details[80];
    char **option_ptr;
    int menu_row = GREET_ROW + 4;
    int menu_column = 8;
    
    //标题
    mvprintw(TITLE_ROW, WIN_WIDTH/2-8, "书 架 管 理 系 统");
    move(DETAILS_ROW+1, menu_column-1);
    whline(stdscr, ACS_HLINE, 54);

    //欢迎行
    if(strlen(login_user) <= 8){
        sprintf(greet, "欢迎: %s", login_user);
    }else{
        strncpy(greet+10, login_user, 8);
        strcpy(greet+18, "***");
    }
    mvprintw(GREET_ROW, menu_column+19, greet);

    //信息行
    sprintf(details, "书架%d个,图书%d册 在读%d册 借出%d册 未归档%d册",\
            gui_sc.shelves_all, gui_bc.books_all, gui_bc.books_on_reading, gui_bc.books_borrowed, gui_bc.books_unsorted);
    mvprintw(DETAILS_ROW, menu_column+4, details);

    //显示菜单
    option_ptr = menu_option;
    while(*option_ptr){
        mvprintw(menu_row, WIN_WIDTH/2-8, *option_ptr);
        menu_row += 3;
        option_ptr++;
    }

    refresh();
}

ui_menu_e ncgui_display_lookthrough_page(void)
{
    return(display_lookthrough_page(0));
}

ui_menu_e ncgui_display_searchbook_page(void)
{
    int max_str_len  = BOOK_NAME_LEN + AUTHOR_NAME_LEN + 31;//31:author等关键字占用的空间
    int kb_posx = GREET_ROW+5, kb_posy = 8;
    int kb_hight = 9, kb_width = 52;
    char keyword[BOOK_NAME_LEN + AUTHOR_NAME_LEN + 32] = {0};
    book_entry_t local_book;
    shelf_entry_t local_shelf;
    int shelfno = NON_SENSE_INT;
    ui_menu_e rt_menu=menu_non_sense_e; 
    WINDOW *kbwin;
    slider_t kb_slider;
    int kb_key = LOCAL_KEY_NON_SENSE;
    int op_key = LOCAL_KEY_NON_SENSE;

    //初始化搜索结构
    memset(&local_book, '\0', sizeof(book_entry_t));
    local_book.code[0] = NON_SENSE_INT;

    //初始化界面
    kbwin = newwin(kb_hight-2, kb_width-2, kb_posx+1, kb_posy+1);
    draw_searchbook_page_framework(kbwin, local_book.code[0]);

    //获取用户输入并分析
    do{
        wclear(kbwin);
        wrefresh(kbwin);
        curs_set(1);
        wgetnstr(kbwin, keyword, max_str_len);
        if(cursor_state == 0)curs_set(0);
    }while(handle_searchbook_keyword(keyword, &local_book) == 0);
    show_searchbox_info(&local_book);

    //滑块初始位置
    kb_slider.win = stdscr;
    kb_slider.start_posx = kb_posx+kb_hight;
    kb_slider.start_posy = kb_posy - 3;
    kb_slider.nstep = 1;
    kb_slider.current_row = kb_slider.last_row = 0;
    kb_slider.max_row = 1;
    move_slider(&kb_slider, 0);

    //获取用户选择
    while(kb_key != KEY_BACKSPACE)
    {
        kb_key = get_choice(page_searchbook_e, &kb_slider, kb_slider.max_row);
        if(kb_key == KEY_ENTER || kb_key == '\n'){
            switch(kb_slider.current_row){
                case sb_option_select_shelf:
                    ncgui_clear_all_screen();
                    if((shelfno = display_lookthrough_page(1)) != NON_SENSE_INT)//用户锁定书架
                        local_book.code[0] = shelfno;
                    draw_searchbook_page_framework(kbwin, local_book.code[0]);
                    mvprintw(kb_posx+1, kb_posy+1, "%s", keyword);
                    refresh();
                    move_slider(&kb_slider, 0);
                    break;
                case sb_option_finish:
                    local_shelf.code = local_book.code[0];
                    (void)client_searching_book(BREAK_LIMIT_INT, &local_book);//先获取一次以便得到检索总数
                    while(op_key != KEY_BACKSPACE){ 
                        ncgui_clear_all_screen();
                        op_key = display_bookinfo_page(&local_shelf, 0, &local_book);
                    }
                    kb_key = KEY_BACKSPACE;
                    break;
                default: break;
            }
        }
    }

    //扫尾工作
    delwin(kbwin);

    return(rt_menu);
}

ui_menu_e ncgui_display_buildshelf_page(void)
{
    WINDOW *nbwin, *boxwin, *padwin;
    shelf_entry_t local_shelf;
    int df_posx = GREET_ROW-1, df_posy = 8;
    int df_hight = 8, df_width = 40;
    char temp_str[SHELF_NAME_LEN+1] = {0};
    char *tsptr;
    int nfloors = 1, i = 0;
    int select_key = LOCAL_KEY_NON_SENSE;
    unsigned char depth_record[MAX_FLOORS] = {0};
    int rt_menu = menu_non_sense_e;

    //显示标题
    mvprintw(TITLE_ROW, WIN_WIDTH/2-8, menu_option[menu_build_shelf_e]);
    mvprintw(df_posx+1, df_posy, "书架名称:");

    //绘制输入区
    nbwin = newwin(df_hight-2, df_width-2, df_posx+1, df_posy+11);
    boxwin = subwin(stdscr, df_hight, df_width, df_posx, df_posy+10);
    box(boxwin, ACS_VLINE, ACS_HLINE);
    touchwin(stdscr);
    refresh();

    //获取用户输入书架名称
    do{
        (void)get_confirm("给它起个名字?", "Certain");
        do{
            wclear(nbwin);
            wrefresh(nbwin);
            memset(temp_str, '\0', SHELF_NAME_LEN+1);
            curs_set(1);
            mvwgetnstr(nbwin, 0, 0, temp_str, SHELF_NAME_LEN);
            if(cursor_state == 0)curs_set(0);
            wclear(nbwin);
            wrefresh(nbwin);
            //过滤字符串首尾连续的空格
            tsptr = string_filter(temp_str, '?');
            mvwprintw(nbwin, 0, 0, tsptr);
            wrefresh(nbwin);
        }while(!get_confirm("确定?", "No"));
    }while(!check_shelf_name(tsptr));//名称查重
    strcpy(local_shelf.name, tsptr);

    //确定书架层数
    do{
        nfloors = 1;
        select_key = LOCAL_KEY_NON_SENSE;
        (void)get_confirm("它多高?", "Certain");
        curs_set(1);
        while(select_key != KEY_ENTER && select_key != '\n'){
            mvprintw(df_posx+5, df_posy+1, "共%02d层", nfloors);
            move(df_posx+5, df_posy+4);
            refresh();
            select_key = get_choice(page_buildshelf_e, NULL, 0);
            if(select_key == KEY_UP){
                ++nfloors > MAX_FLOORS ? nfloors = 1: 0;
            }else if(select_key == KEY_DOWN){
                --nfloors < 1? nfloors = MAX_FLOORS: 0;
            }
        }
        if(cursor_state == 0)curs_set(0);
    }while(!get_confirm("确定?", "No"));
    local_shelf.nfloors = nfloors;

    //填入详细信息
    padwin = newpad(PAD_WIDTH,  PAD_WIDTH);
    draw_shelf_profile(padwin, nfloors, NULL);
    do{
        set_shelf_parameter(padwin, nfloors, depth_record);
    }while(!get_confirm("确定?", "No"));
    for(i = 0; i < nfloors; i++)
        local_shelf.ndepth[i] = depth_record[i];

    //开始打造
    memset(temp_str, '\0', SHELF_NAME_LEN+1);
    if(get_confirm("开始打造吗?", "Yes")){
        if(client_build_shelf(&local_shelf, temp_str)) error_line("新书架打造成功!"); else
            error_line(temp_str);
        sleep(2);
    }else{
        if(get_confirm("重新打造吗?", "No"))
            rt_menu = menu_cycle_e;
    }

    //清除编辑框
    wclear(boxwin);
    delwin(boxwin);
    wclear(nbwin);
    delwin(nbwin);
    wclear(padwin);
    delwin(padwin);

    return(rt_menu);
}

static void draw_shelf_profile(WINDOW *win, int nfloors, unsigned char depthArr[])
{
    WINDOW *shelfbox;
    WINDOW *footbox1, *footbox2;
    int df_posx = 1, df_posy = 1;
    int i;

    //绘制出书架轮廓
    shelfbox = subwin(win, nfloors*2+1, 12, df_posx, df_posy);
    box(shelfbox, ACS_VLINE, ACS_HLINE);
    footbox1 = subwin(win, 2, 2, df_posx+nfloors*2, df_posy);
    box(footbox1, ACS_VLINE, ACS_HLINE);
    footbox2 = subwin(win, 2, 2, df_posx+nfloors*2, df_posy+10);
    box(footbox2, ACS_VLINE, ACS_HLINE);

    for(i = 1; i < nfloors; i++)
    {
        wmove(win, df_posx+2*i, df_posy+1);
        whline(win, ACS_HLINE, 10);
        if(i%3 == 1)
            mvwprintw(win, df_posx+2*nfloors+1-2*i, df_posy+12, "%2dF", i);
    }
    if(i%3 == 1)
        mvwprintw(win, df_posx+2*nfloors+1-2*i, df_posy+12, "%2dF", i);

    if(depthArr != NULL){
        for(i = 1; i <= nfloors; i++)
            mvwprintw(win, nfloors*2+2-i*2, df_posy+5, "%2dR", depthArr[i-1]);
    }

    delwin(shelfbox);
    delwin(footbox1);
    delwin(footbox2);
}

static void set_shelf_parameter(WINDOW *win, int nfloors, unsigned char depthArr[])
{
    int pad_startx = GREET_ROW + 8, pad_starty = 36;
    int pad_show_hight = 7, pad_show_width = 16;
    int start_hight;
    int profile_hight;
    int select_key;
    int curs_floor, curs_posy;
    int df_posy = 1;

    //标注操作提示
    mvwprintw(stdscr, pad_startx+2, 22,  "↕ 层间跳跃"); 
    mvwprintw(stdscr, pad_startx+4, 22,  "↔ 调整排数"); 
    refresh();

    //支持翻页和调整书架深度
    mvwprintw(win, 2*nfloors, df_posy+5, "%dR", depthArr[0]);
    wmove(win, 2*nfloors, df_posy+5);
    curs_floor = 1;
    curs_posy = df_posy+5;
    profile_hight = 2*nfloors + 2;
    select_key = LOCAL_KEY_NON_SENSE;
    if(profile_hight <= pad_show_hight)
        start_hight = 0;
    else
        start_hight = profile_hight - pad_show_hight;
    curs_set(1);
    while(select_key != KEY_ENTER && select_key != '\n'){
        if(profile_hight <= pad_show_hight)start_hight = 0;
        prefresh(win, start_hight, 0, pad_startx, pad_starty,\
                pad_startx+pad_show_hight, pad_starty+pad_show_width);
        select_key = get_choice(page_buildshelf_e, NULL, 0);
        switch(select_key)
        {
            case KEY_DOWN:
                start_hight += 2;
                start_hight > profile_hight-pad_show_hight? start_hight=profile_hight-pad_show_hight: 0;
                --curs_floor < 1? curs_floor = 1: 0;
                break;
            case KEY_UP:
                start_hight -= 2;
                start_hight <= 0? start_hight=0 : 0;
                ++curs_floor > nfloors? curs_floor = nfloors: 0;
                break;
            case KEY_LEFT:
                --depthArr[curs_floor-1] < 0 ? depthArr[curs_floor-1]=0 : 0;
                break;
            case KEY_RIGHT:
                ++depthArr[curs_floor-1] > MAX_DEPTH ? depthArr[curs_floor-1]=MAX_DEPTH : 0;
                break;
            default: break;
        }
        mvwprintw(win, profile_hight-curs_floor*2, curs_posy, "%dR", depthArr[curs_floor-1]);
        wmove(win, profile_hight-curs_floor*2, curs_posy);
    }
    if(cursor_state == 0)curs_set(0);
}

static int check_shelf_name(char *name)
{
    int i;
    shelf_entry_t local_shelf;

    if(strlen(name) == 0){
        error_line("书架名为空");
        sleep(1);
        clear_line(NULL, PROMPT_ROW, 1, 1, WIN_WIDTH-2);
        return(0);
    }
    for(i = 1; i <= MAX_SHELF_NUM; i++)
    {
        if(clidb_shelf_exists(i)){
            clidb_shelf_get(i, &local_shelf);
            if(!strcmp(local_shelf.name, name)){
                error_line("此书架已存在");
                sleep(1);
                clear_line(NULL, PROMPT_ROW, 1, 1, WIN_WIDTH-2);
                return(0);
            }
        }
    }

    return(1);
}

void draw_searchbook_page_framework(WINDOW *win, int shelfno)
{
    int kb_posx = GREET_ROW+5, kb_posy = 8;
    int kb_hight = 9, kb_width = 52;
    WINDOW *boxwin;
    shelf_entry_t local_shelf;

    ncgui_clear_all_screen();
    mvprintw(TITLE_ROW, WIN_WIDTH/2-8, menu_option[menu_search_book_e]); 
    print_searchbook_prompt();

    //显示菜单(锁定书架/开始检索)
    mvprintw(kb_posx + kb_hight, kb_posy, *searchbook_option);
    mvprintw(kb_posx + kb_hight + 1, kb_posy, *(searchbook_option+1));
    if(shelfno != NON_SENSE_INT){
        if(clidb_shelf_get(shelfno, &local_shelf))
            mvprintw(kb_posx+ kb_hight, kb_posy + 12, "<%s>", local_shelf.name);
    }else{
            mvprintw(kb_posx+ kb_hight, kb_posy + 12, "<无>");
    }

    boxwin = subwin(stdscr, kb_hight, kb_width, kb_posx, kb_posy);
    box(boxwin, ACS_VLINE, ACS_HLINE);
    touchwin(stdscr);
    refresh();

    delwin(boxwin);
    wrefresh(win);
}

static ui_menu_e display_lookthrough_page(int select_mode)
{
    int pad_posx = GREET_ROW+2, pad_posy = 8;
    int first_line = 0, logic_rows = 0, option_offset = 0;
    int lt_key = LOCAL_KEY_NON_SENSE, op_key = LOCAL_KEY_NON_SENSE;
    shelf_entry_t local_shelf;
    book_count_t local_count;
    slider_t lt_slider, op_slider;
    WINDOW *padwin, *opwin;
    ui_menu_e rt_menu = menu_non_sense_e; 
    
    if(select_mode == 1)rt_menu = NON_SENSE_INT; 

    //标题
    mvprintw(TITLE_ROW, WIN_WIDTH/2-8, menu_option[menu_look_through_e]); 

    //分割竖线
    move(WIN_HIGHT/4, WIN_WIDTH/2);
    wvline(stdscr, ACS_VLINE, WIN_HIGHT/10*6);

    refresh();

    //shelves pad填充内容并初始化滑块
    if(!pad_fill_shelves_info(&padwin, &lt_slider, &logic_rows))
        return(menu_non_sense_e);

    //滑块初始位置
    move_slider(&lt_slider, 0);
    clidb_shelf_get(clidb_shelf_realno(first_line+lt_slider.current_row+1), &local_shelf);
    client_shelf_count_book(local_shelf.code, &local_count);
    show_shelf_info(pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, &local_shelf, &local_count);

    //获取用户输入
    while(lt_key != KEY_BACKSPACE){
        prefresh(padwin, first_line, 0, pad_posx, pad_posy, pad_posx+PAD_BOXED_HIGHT-1, PAD_BOXED_WIDTH);
        lt_key = get_choice(page_lookthrough_e, &lt_slider, logic_rows);
        if(lt_key == KEY_UP || lt_key == KEY_DOWN){
            //获取书架信息并显示
            clidb_shelf_get(clidb_shelf_realno(lt_slider.current_row+1), &local_shelf);
            client_shelf_count_book(local_shelf.code, &local_count);
            show_shelf_info(pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, &local_shelf, &local_count);
            //计算出滚动屏幕需要的first_line值
            scroll_pad(&lt_slider, logic_rows, &first_line);
        }
        //用户输入ENTER显示选择菜单
        if(lt_key == KEY_ENTER || lt_key == '\n'){
            if(select_mode == 1){//用于获取书架编号的模式
                if(get_confirm("在此书架范围内进行检索?", "Yes"))
                    return(local_shelf.code);
            }else{
                move_slider(&lt_slider, 1);
                option_offset = 0;
                if(gui_bc.books_unsorted > 0)option_offset = 5;//存在待招领图书
                if(local_count.books_all-local_count.books_unsorted <= 0)option_offset++;//空书架不允许打开
                draw_lt_option_box(&opwin, &op_slider, lookthrough_option+option_offset);
                option_offset %= 5;
                while(op_key != KEY_BACKSPACE){ 
                    prefresh(padwin, first_line, 0, pad_posx, pad_posy, pad_posx+PAD_BOXED_HIGHT-1, PAD_BOXED_WIDTH);
                    //获取用户选择
                    if(op_key != KEY_ENTER)op_key = get_choice(page_lookthrough_e, &op_slider, op_slider.max_row);
                    if(op_key == KEY_ENTER || op_key == '\n'){
                        switch(op_slider.current_row+option_offset){
                            case lt_option_open:
                                op_key = display_bookinfo_page(&local_shelf, 0, NULL);//修改op_key的值完成翻页功能
                                break;
                            case lt_option_newbook:
                                op_key = add_newbook_page(&local_shelf);
                                break;
                            case lt_option_profile:
                                op_key = display_shelfprofile_page(&local_shelf);
                                break;
                            case lt_option_collectbook:
                                op_key = display_bookinfo_page(&local_shelf, 1, NULL);
                                break;
                            case lt_option_destroy:
                                op_key = delete_shelf_page(&local_shelf, &local_count);
                                if(op_key == LOCAL_KEY_CERTAIN_MEANING){
                                    lt_key = op_key = KEY_BACKSPACE;//返回主界面
                                    rt_menu = menu_cycle_e;//请求再次返回本界面
                                }
                                break;
                            default: break;
                        }
                        if(op_key == LOCAL_KEY_NON_SENSE){
                            op_key = KEY_BACKSPACE;//默认返回上一界面
                            move_slider(&lt_slider, 0);
                            ncgui_clear_all_screen();
                            mvprintw(TITLE_ROW, WIN_WIDTH/2-8, menu_option[menu_look_through_e]); 
                            move(WIN_HIGHT/4, WIN_WIDTH/2);
                            wvline(stdscr, ACS_VLINE, WIN_HIGHT/10*6);
                            refresh();
                        }
                    }
                }
                op_key = LOCAL_KEY_NON_SENSE;
                destroy_lt_option_box(&opwin);
                move_slider(&lt_slider, 0);
                clidb_shelf_get(clidb_shelf_realno(first_line+lt_slider.current_row+1), &local_shelf);
                client_shelf_count_book(local_shelf.code, &local_count);
                show_shelf_info(pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, &local_shelf, &local_count);
                refresh();
            }
        }
    }

    delwin(padwin);

    //同步信息
    if(select_mode != 1){
        client_shelves_count_sync(&gui_sc);
        client_books_count_sync(&gui_bc);
    }

    return(rt_menu);
}

static int display_shelfprofile_page(shelf_entry_t *user_shelf)
{
    WINDOW *padwin;
    int select_key = LOCAL_KEY_NON_SENSE;

    //绘制书架
    padwin = newpad(PAD_WIDTH, PAD_WIDTH);
    draw_shelf_profile(padwin, user_shelf->nfloors, user_shelf->ndepth);

    int pad_startx = GREET_ROW + 6, pad_starty = WIN_WIDTH-PAD_BOXED_WIDTH+2;
    int pad_show_hight = 7, pad_show_width = 16;
    int profile_hight, start_hight;

    //显示书架
    profile_hight = 2*user_shelf->nfloors + 2;
    select_key = LOCAL_KEY_NON_SENSE;
    if(profile_hight <= pad_show_hight)
        start_hight = 0;
    else
        start_hight = profile_hight - pad_show_hight;
    clear_line(NULL, pad_startx, pad_starty-4, 9, PAD_BOXED_WIDTH);//清除选择框
    error_line("↕ 层间跳跃");
    while(select_key != KEY_BACKSPACE){
        if(profile_hight <= pad_show_hight)start_hight = 0;
        prefresh(padwin, start_hight, 0, pad_startx, pad_starty, pad_startx+pad_show_hight, pad_starty+pad_show_width);
        select_key = get_choice(page_buildshelf_e, NULL, 0);
        switch(select_key)
        {
            case KEY_DOWN:
                start_hight += 2;
                start_hight > profile_hight-pad_show_hight? start_hight=profile_hight-pad_show_hight: 0;
                break;
            case KEY_UP:
                start_hight -= 2;
                start_hight <= 0? start_hight=0 : 0;
                break;
            default: break;
        }
    }
    clear_line(NULL, PROMPT_ROW, 1, 1, PAD_WIDTH-2);

    return(LOCAL_KEY_NON_SENSE);
}

static void show_searchbox_info(book_entry_t *local_book)
{
    int kb_posx = GREET_ROW+6, kb_posy = 8;
    int str_limit = 30;
    char str_show[36] = {0};

    //检索框显示书名关键字
    if(strlen(local_book->name)-1 > str_limit){
        strncpy(str_show, local_book->name+1, str_limit);
        strcat(str_show, "...");
    }else{
        strcpy(str_show, local_book->name+1);
    }
    switch(local_book->name[0]){
        case '0':
            break;
        case '-':
            mvprintw(kb_posx++, kb_posy+1, "书名 ≈ %s", str_show);
            if(local_book->author[0] != '0')
                mvprintw(kb_posx++, kb_posy+1, "     or   ");
            break;
        case '+':
            mvprintw(kb_posx++, kb_posy+1, "书名 ≈ %s", str_show);
            if(local_book->author[0] != '0')
                mvprintw(kb_posx++, kb_posy+1, "     and   ");
            break;
        case '=':
            mvprintw(kb_posx++, kb_posy+1, "书名 = %s", str_show);
            if(local_book->author[0] != '0')
                mvprintw(kb_posx++, kb_posy+1, "     and   ");
            break;
        default: break;
    }
    memset(str_show, '\0', 36);

    //作者关键字
    if(strlen(local_book->author)-1 > str_limit){
        strncpy(str_show, local_book->author+1, str_limit);
        strcat(str_show, "...");
    }else{
        strcpy(str_show, local_book->author+1);
    }
    switch(local_book->author[0]){
        case '0':
            break;
        case '-':
            mvprintw(kb_posx++, kb_posy+1, "作者 ≈ %s", str_show);
            if(local_book->label[0] != '0')
                mvprintw(kb_posx++, kb_posy+1, "     or   ");
            break;
        case '+':
            mvprintw(kb_posx++, kb_posy+1, "作者 ≈ %s", str_show);
            if(local_book->label[0] != '0')
                mvprintw(kb_posx++, kb_posy+1, "     and   ");
            break;
        case '=':
            mvprintw(kb_posx++, kb_posy+1, "作者 = %s", str_show);
            if(local_book->label[0] != '0')
                mvprintw(kb_posx++, kb_posy+1, "     and   ");
            break;
        default: break;
    }
    memset(str_show, '\0', 36);

    //标签关键字
    if(strlen(local_book->label)-1 > str_limit){
        strncpy(str_show, local_book->label+1, str_limit);
        strcat(str_show, "...");
    }else{
        strcpy(str_show, local_book->label+1);
    }
    switch(local_book->label[0]){
        case '0':
            break;
        case '-':
            mvprintw(kb_posx++, kb_posy+1, "标签 ≈ %s", str_show);
            break;
        case '+':
            mvprintw(kb_posx++, kb_posy+1, "标签 ≈ %s", str_show);
            break;
        default: break;
    }

    refresh();
}

static int handle_searchbook_keyword(char *keyword, book_entry_t *user_book)
{
    char *eptr = NULL;
    char *kwptr = keyword;
    char keyword_copy[BOOK_NAME_LEN + AUTHOR_NAME_LEN + 32] = {0};
    char sub_keyword[BOOK_NAME_LEN + AUTHOR_NAME_LEN + 32] = {0};
    char sub_value[BOOK_NAME_LEN + AUTHOR_NAME_LEN + 32] = {0};
    int sub_len = BOOK_NAME_LEN+AUTHOR_NAME_LEN+31;
    int i = 0, rt = 0;

    strcpy(keyword_copy, keyword);
    kwptr = string_filter(keyword_copy, '\0');
    user_book->name[0] = '0';
    user_book->author[0] = '0';
    user_book->label[0] = '0';

    eptr = strchr(keyword_copy, '=');
    if(eptr == NULL){//模糊搜索
        user_book->name[0] = '-';
        strncpy(user_book->name+1, kwptr, BOOK_NAME_LEN-1);
        user_book->author[0] = '-';
        strncpy(user_book->author+1, kwptr, AUTHOR_NAME_LEN-1);
        user_book->label[0] = '-';
        strncpy(user_book->label+1, kwptr, LABEL_NAME_LEN-1);
        rt++;
    }else{//混合搜索(精确+范围)
        while(string_splitter(sub_keyword, sub_len, keyword, i++, ';')){
            if(string_splitter(sub_value, sub_len, sub_keyword, 0, '=')){
                if(strlen(sub_value) == 1){
                    switch(sub_value[0]){
                        case 'b':
                            if(user_book->name[0] == '=')break;//已有精确匹配存在
                            string_splitter(sub_value, sub_len, sub_keyword, 1, '=');            
                            user_book->name[0] = '+';
                            strcpy(user_book->name+1, sub_value);
                            rt++;
                            break;
                        case 'a':
                            if(user_book->name[0] == '=')break;
                            string_splitter(sub_value, sub_len, sub_keyword, 1, '=');            
                            user_book->author[0] = '+';
                            strcpy(user_book->author+1, sub_value);
                            rt++;
                            break;
                        case 'l':
                            if(user_book->name[0] == '=')break;
                            string_splitter(sub_value, sub_len, sub_keyword, 1, '=');            
                            user_book->label[0] = '+';
                            strcpy(user_book->label+1, sub_value);
                            rt++;
                            break;
                        default:
                            break;
                    }
                }else{
                    if(!strcmp(sub_value, "book")){
                        string_splitter(sub_value, sub_len, sub_keyword, 1, '=');            
                        user_book->name[0] = '=';
                        strcpy(user_book->name+1, sub_value);
                        rt++;
                    }else if(!strcmp(sub_value, "author")){
                        string_splitter(sub_value, sub_len, sub_keyword, 1, '=');            
                        user_book->author[0] = '=';
                        strcpy(user_book->author+1, sub_value);
                        rt++;
                    }
                }
            }
        }
    }

    /*fprintf(stderr, "%s\n", user_book->name);*/
    /*fprintf(stderr, "%s\n", user_book->author);*/
    /*fprintf(stderr, "%s\n", user_book->label);*/
    /*fprintf(stderr, "%d\n", rt);*/
    /*getch();*/

    return(rt);
}

static void print_searchbook_prompt(void)
{
    int kb_posx = GREET_ROW, kb_posy = 10;

    mvprintw(kb_posx, kb_posy,   "模糊: keyword");
    mvprintw(kb_posx+1, kb_posy, "范围: b/a/l = keyword  ");
    mvprintw(kb_posx+2, kb_posy, "精确: book/author = keyword ");
    mvprintw(kb_posx+3, kb_posy, "备注: 1. 请使用英文逗号分割多关键字");
    mvprintw(kb_posx+4, kb_posy, "      2. (b/book:书名 a/author:作者 l:标签)");

    refresh();
}

static int delete_shelf_page(shelf_entry_t *user_shelf, book_count_t *sc)
{
    WINDOW *dswin;
    slider_t op_slider;
    int op_key = LOCAL_KEY_NON_SENSE;


    if(!get_confirm("即将删除书架,继续?","No"))
        return(LOCAL_KEY_NON_SENSE);
    if(sc->books_all-sc->books_unsorted > 0){//书架上有图书
        draw_lt_option_box(&dswin, &op_slider,shelfdel_option);
        (void)get_confirm("书架上的图书如何处理?","Certain");
        while(op_key != KEY_BACKSPACE){
            op_key = get_choice(page_lookthrough_e, &op_slider, op_slider.max_row);
            if(op_key == '\n' || op_key == KEY_ENTER){
                switch(op_slider.current_row)
                {
                    case ds_option_collect:
                        if(client_shelf_unsorted_books(user_shelf->code))
                            error_line("设置图书招领成功!可通过拾遗操作进行招领");
                        else
                            error_line("设置图书招领失败...");
                        break;
                    case ds_option_abandon:
                        if(client_shelf_abandon_books(user_shelf->code))
                            error_line("弃置图书成功!");
                        else
                            error_line("弃置图书失败...");
                        break;
                    default:
                        break;
                }
                op_key = KEY_BACKSPACE;
            }
        }
        destroy_lt_option_box(&dswin);
        sleep(1);
    }

    if(client_shelf_delete_itself(user_shelf->code)){
        op_key = LOCAL_KEY_CERTAIN_MEANING;
        error_line("拆除书架成功!");
    }else{
        op_key = LOCAL_KEY_NON_SENSE;
        error_line("拆除书架失败...");
    }

    return(op_key);
}

static int add_newbook_page(shelf_entry_t *user_shelf)
{
    int pad_posx = GREET_ROW+2, pad_posy = 8;
    char **option_ptr = newbook_option;
    int nb_key = LOCAL_KEY_NON_SENSE;
    slider_t nb_slider;
    book_entry_t local_book;
    int i = 0;
    char errInfo[64] = {0};

    ncgui_clear_all_screen();

    mvprintw(TITLE_ROW, WIN_WIDTH/2-6, "上 架 新 书 ✎"); 
    mvprintw(GREET_ROW, pad_posy, "✰ %s", user_shelf->name);

    //初始化新书结构
    memset(&local_book, '\0', sizeof(book_entry_t));
    local_book.code[0] = user_shelf->code;
    local_book.borrowed = '0';
    local_book.on_reading = '0';

    //初始化上新界面
    while(*option_ptr){
        mvprintw(pad_posx+i, pad_posy, *option_ptr);
        option_ptr++;
        i += 2;
    };

    //滑块初始位置
    nb_slider.win = stdscr;
    nb_slider.start_posx = pad_posx;
    nb_slider.start_posy = pad_posy - 3;
    nb_slider.nstep = 2;
    nb_slider.current_row = nb_slider.last_row = 0;
    nb_slider.max_row = i/2-1;
    move_slider(&nb_slider, 0);
    
    int flag = 1;
    nb_key = KEY_ENTER;//默认输入书名
    while(nb_key != KEY_BACKSPACE){
        if(nb_key != KEY_ENTER)
            nb_key = get_choice(page_newbook_e, &nb_slider, nb_slider.max_row);
        if(nb_key == KEY_BACKSPACE){
            if(!get_confirm("不保存返回?", "No"))
                nb_key = LOCAL_KEY_NON_SENSE;
        }else if(nb_key == KEY_ENTER || nb_key == '\n'){
            switch(nb_slider.current_row)
            {
                case nb_option_name:
                    local_book.code[2] = nb_option_name;
                    get_newbook_info(&local_book);
                    show_newbook_info(&local_book);
                    break;
                case nb_option_author:
                    local_book.code[2] = nb_option_author;
                    get_newbook_info(&local_book);
                    show_newbook_info(&local_book);
                    break;
                case nb_option_nfloor:
                    select_newbook_floor(user_shelf, &local_book, GREET_ROW+6, 18);
                    show_newbook_info(&local_book);
                    break;
                case nb_option_tagging:
                    draw_bi_tagging_box(&local_book, pad_posx+7, pad_posy+10);
                    show_newbook_info(&local_book);
                    break;
                case nb_option_finish:
                    if(get_confirm("数据将被上传,确定?", "No")){
                        if(client_shelf_insert_book(&local_book, errInfo)){
                            error_line("上架图书成功!");
                            if(get_confirm("继续输入?", "Yes")){
                                memset(&local_book, '\0', sizeof(book_entry_t));
                                local_book.code[0] = user_shelf->code;
                                local_book.borrowed = '0';
                                local_book.on_reading = '0';
                                flag = 2;
                            }else{
                                nb_key = KEY_BACKSPACE;
                            }
                        }else{
                            error_line(errInfo);
                        }
                    }
                    break;
                default:
                    break;
            }
            if(flag == 1 ){
                nb_key = KEY_ENTER;
                if(nb_slider.current_row >= nb_option_tagging){
                    nb_key = LOCAL_KEY_NON_SENSE;
                    flag = 0;
                }else{
                    nb_slider.last_row = nb_slider.current_row;
                    nb_slider.current_row++;
                    move_slider(&nb_slider, 0);
                }
            }else if(flag == 2){
                flag--;
                nb_key = KEY_ENTER;
                nb_slider.last_row = nb_slider.current_row;
                nb_slider.current_row = 0;
                move_slider(&nb_slider, 0);
            }
        }
    }

    return(LOCAL_KEY_NON_SENSE);
}

static void get_newbook_info(book_entry_t *user_book)
{
    int df_posx = GREET_ROW+1, df_posy = 8;
    int df_hight = 9, df_width = 40;
    int box_posx, box_posy, box_hight, box_width;
    int max_str_len = BOOK_NAME_LEN;
    char temp_str[BOOK_NAME_LEN+1];
    char *tsptr;
    WINDOW *nbwin, *boxwin;

    switch(user_book->code[2]){
        case nb_option_name:
            box_posx = df_posx; box_posy=df_posy+8; box_hight = df_hight; box_width = df_width;
            break;
        case nb_option_author:
            max_str_len = AUTHOR_NAME_LEN;
            box_posx = df_posx+2; box_posy=df_posy+8; box_hight = df_hight-2; box_width = df_width;
            break;
        default:
            break;
    }

    nbwin = newwin(box_hight-2, box_width-2, box_posx+1, box_posy+1);
    boxwin = subwin(stdscr,  box_hight, box_width, box_posx, box_posy);
    box(boxwin, ACS_VLINE, ACS_HLINE);
    touchwin(stdscr);
    refresh();

    //获取用户输入
    curs_set(1);
    if(user_book->code[2] == nb_option_name)
        wgetnstr(nbwin, temp_str, max_str_len);
    else if(user_book->code[2] == nb_option_author)
        wgetnstr(nbwin, temp_str, max_str_len);
    if(cursor_state == 0)curs_set(0);

    //过滤字符串首尾连续的空格
    tsptr = string_filter(temp_str, '?');

    if(user_book->code[2] == nb_option_name)
        strcpy(user_book->name, tsptr);
    else if(user_book->code[2] == nb_option_author)
        strcpy(user_book->author, tsptr);

    //清除编辑框
    wclear(boxwin);
    wclear(nbwin);
    delwin(boxwin);
    delwin(nbwin);
    refresh();
}

static void select_newbook_floor(shelf_entry_t *user_shelf, book_entry_t *user_book, int posx, int posy)
{
    int key = 0;
    int max_floor = user_shelf->nfloors;
    int cur_floor = 1, cur_depth = 1, cur_cursor = 0;
    int pad_posx = posx, pad_posy = posy;

    keypad(stdscr, TRUE);
    cbreak();
    noecho();

    curs_set(1);
    while(key != KEY_ENTER && key != '\n'){
        mvprintw(pad_posx, pad_posy, "%02d层 %02d排", cur_floor, cur_depth);
        if(!cur_cursor)
            move(pad_posx, pad_posy+1);
        else
            move(pad_posx, pad_posy+6);
        key = getch();
        switch(key){
            case KEY_UP:
                if(!cur_cursor)
                    ++cur_floor > max_floor ? cur_floor = max_floor : 0;
                else
                    ++cur_depth > user_shelf->ndepth[cur_floor-1] ? cur_depth = user_shelf->ndepth[cur_floor-1]: 0;
                break;
            case KEY_DOWN:
                if(!cur_cursor)
                    --cur_floor < 1 ? cur_floor = 1: 0;
                else
                    --cur_depth < 1 ? cur_depth = 1: 0;
                break;
            case KEY_LEFT:
            case KEY_RIGHT:
                cur_depth = 1;
                cur_cursor ^= 1;
                break;
            default: break;
        }
    }

    if(cursor_state == 0)curs_set(0);
    echo();
    nocbreak();
    keypad(stdscr, FALSE);

    //更新数据
    user_book->code[1] = (int)cur_floor<<4;
    user_book->code[1] |= cur_depth;
}

static void show_newbook_info(book_entry_t *user_book)
{
    int df_posx = GREET_ROW+2, df_posy = 16;
    int df_hight = 16, df_width = 40;
    int str_limit = 30;
    char str_show[40]={0};
    char slabel[MAX_LABEL_NUM/2][2*LABEL_NAME_LEN+1];
    int str_len = 0, j = 0;;

    memset(slabel, '\0', sizeof(slabel));
    clear_line(NULL, df_posx, df_posy, df_hight, df_width);

    //显示书名
    if(*user_book->name){
        str_len = strlen(user_book->name);
        strcat(str_show, " 《");
        if(str_len > str_limit){
            strncpy(str_show+4, user_book->name, str_limit);
            strcat(str_show, "...》");
        }else{
            strncpy(str_show+4, user_book->name, str_len);
            strcat(str_show, "》");
        }
        mvprintw(df_posx, df_posy, "%s", str_show);
        memset(str_show, '\0', sizeof(str_show));
    }

    //显示作者名
    if(*user_book->author){
        str_len = strlen(user_book->author);
        if(str_len > str_limit){
            strncpy(str_show, user_book->author, str_limit);
            strcat(str_show, "...");
        }else{
            strncpy(str_show, user_book->author, str_len);
        }
        mvprintw(df_posx+2, df_posy+2, "%s", str_show);
        memset(str_show, '\0', sizeof(str_show));
    }

    //显示层号
    if(user_book->code[1] > 0){
        sprintf(str_show, "%02d层 %02d排", user_book->code[1]>>4, user_book->code[1]&0x0F);
        mvprintw(df_posx+4, df_posy+2,"%s", str_show);
    }

    //显示标签 
    if(*user_book->label){
        split_labels(user_book->label, slabel);
        while(slabel[j][0]){
            mvprintw(df_posx+6+2*j, df_posy+1, "☘  %s", slabel[j]);
            j++;
        }
    }

    refresh();
}

static int display_bookinfo_page(shelf_entry_t *user_shelf, int collect_mode, book_entry_t *search_entry)
{ 
    static int local_shelfno = NON_SENSE_INT; 
    static int local_bookno = NON_SENSE_INT;
    static int book_page_cnt = 0;
    static int bcollect = 0;
    static char new_mark = 0;

    int search_mode = 0;
    int shelfno = user_shelf->code, mode_shelfno;
    WINDOW *padwin, *opwin;
    int pad_posx = GREET_ROW+2, pad_posy = 8, first_line = 0;
    int nbooks, logic_rows, cur_dbm_pos = 0;
    book_entry_t local_book;
    slider_t bi_slider, op_slider;
    int bi_key =  LOCAL_KEY_NON_SENSE;
    int op_key = LOCAL_KEY_NON_SENSE, rt_key = LOCAL_KEY_NON_SENSE;
    int option_offset = 0;

    //不支持此类参数
    if(collect_mode == 1 && search_entry != NULL)return(rt_key);

    if(search_entry != NULL)search_mode = 1;
    //书架变更,修改记录   
    if(search_mode){   //case 1:进入检索模式
        if(new_mark != search_entry->encoding_time[0]){
            new_mark = search_entry->encoding_time[0];
            local_bookno = NON_SENSE_INT;
            book_page_cnt = 0;
            clidb_book_reset();
        }
    }else if((local_shelfno != shelfno && bcollect == collect_mode) || bcollect != collect_mode){//2:浏览书架时,变更了书架 3:浏览图书/拾遗/检索切换 
        local_shelfno = shelfno;
        bcollect = collect_mode;
        local_bookno = NON_SENSE_INT;
        book_page_cnt = 0;
        clidb_book_reset();
    }

    //初始化界面
    mode_shelfno = local_shelfno;
    if(!search_mode)
        mvprintw(GREET_ROW, pad_posy, "✰ %s", user_shelf->name);//显示当前书架
    padwin = newpad(PAD_HIGHT, PAD_WIDTH);
    clear_line(NULL, pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, PAD_BOXED_HIGHT, PAD_BOXED_WIDTH);

    //拾遗、检索界面特殊处理
    if(bcollect == 1){
        clear_line(NULL, TITLE_ROW, 1, 1, WIN_WIDTH-2);
        mvprintw(TITLE_ROW, WIN_WIDTH/2-6, " 招 领 处 ♻ "); 
        mode_shelfno = NON_SENSE_INT;//取特殊值
    }
    if(search_mode){
        clear_line(NULL, TITLE_ROW, 1, 1, WIN_WIDTH-2);
        mvprintw(TITLE_ROW, WIN_WIDTH/2-8, " 检 索 结 果 ♨ "); 
        mvprintw(GREET_ROW, pad_posy, "%s", search_entry->encoding_time+1);
        move(WIN_HIGHT/4, WIN_WIDTH/2);
        wvline(stdscr, ACS_VLINE, WIN_HIGHT/10*6);
    }
    refresh();

    //初始化书籍信息
    if((nbooks = next_page_bookinfo(padwin, &mode_shelfno, &local_bookno, search_entry)) == -1)return(rt_key);
    if(nbooks == 0){
        if(search_mode){
            rt_key = KEY_BACKSPACE;
            sleep(2);
        }
        return(rt_key);
    }

    //计算实际移动的和逻辑上的最大值
    if(nbooks > PAD_BOXED_HIGHT - 1)
        bi_slider.max_row = PAD_BOXED_HIGHT - 1;
    else
        bi_slider.max_row = nbooks - 1;
    logic_rows = nbooks - 1;

    //滑块初始位置
    bi_slider.win = stdscr;
    bi_slider.start_posx = pad_posx;
    bi_slider.start_posy = pad_posy - 3;
    bi_slider.nstep = 1;
    bi_slider.current_row = bi_slider.last_row = 0;
    move_slider(&bi_slider, 0);
    clidb_book_peek(&local_book, book_page_cnt*PAD_HIGHT+0);
    show_book_info(pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, &local_book, search_mode);

    //获取用户输入
    while(bi_key != KEY_BACKSPACE){
        prefresh(padwin, first_line, 0, pad_posx, pad_posy, pad_posx+PAD_BOXED_HIGHT-1, PAD_BOXED_WIDTH);
        bi_key = get_choice(page_bookinfo_e, &bi_slider, logic_rows);
        if(bi_key != KEY_BACKSPACE){
            //获取图书信息并显示
            clear_line(NULL, Q_ROW, 1, 2, WIN_WIDTH-2);//清空提示行和错误信息行
            cur_dbm_pos = book_page_cnt*PAD_HIGHT+bi_slider.current_row;
            clidb_book_peek(&local_book, cur_dbm_pos);
            show_book_info(pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, &local_book, search_mode);
            if(bi_key == KEY_UP || bi_key == KEY_DOWN)
                scroll_pad(&bi_slider, logic_rows, &first_line);
            if(bi_key == KEY_RIGHT){//向后翻页
                if(next_page_bookinfo(padwin, &mode_shelfno, &local_bookno, search_entry) > 0){
                    book_page_cnt++;
                    rt_key = KEY_ENTER;
                    break;
                }
            }else if(bi_key == KEY_LEFT){//向前翻页
                clidb_book_backward_mode();
                --book_page_cnt < 0 ? book_page_cnt = 0: 0;
                rt_key = KEY_ENTER;
                break;
            }else if(bi_key == KEY_ENTER || bi_key == '\n'){//用户按下回车
                if(bcollect == 1){//拾遗界面特殊处理
                    if(local_book.encoding_time[11] != 'H'){
                        if(get_confirm("拾取本书?","Yes")){
                            (void)get_confirm("选择上架位置","Certain");
                            select_newbook_floor(user_shelf, &local_book, Q_ROW, WIN_WIDTH-PAD_BOXED_WIDTH+8);
                            if(client_shelf_collecting_book(local_shelfno, &local_book, cur_dbm_pos))
                                error_line("拾取图书成功!");
                            else
                                error_line("拾取图书失败...");
                        }
                    }
                }else{
                    if(local_book.encoding_time[0]){//图书未删除
                        if(local_book.on_reading == '1' || local_book.borrowed == '1'){//图书不在架
                            option_offset = 1;
                            draw_lt_option_box(&opwin, &op_slider, bookinfo_option+5);
                        }else{
                            option_offset = 0;
                            draw_lt_option_box(&opwin, &op_slider, bookinfo_option);
                        }
                    }else{
                        op_key = KEY_BACKSPACE;//已删除的图书特殊处理
                    }
                    while(op_key != KEY_BACKSPACE){ 
                        prefresh(padwin, first_line, 0, pad_posx, pad_posy, pad_posx+PAD_BOXED_HIGHT-1, PAD_BOXED_WIDTH);
                        //获取用户选择
                        op_key = get_choice(page_bookinfo_e, &op_slider, op_slider.max_row);
                        if(op_key == KEY_ENTER || op_key == '\n'){
                            switch(op_slider.current_row+option_offset){
                                case bi_option_reading:
                                    if(client_shelf_moving_book(local_shelfno, &local_book, cur_dbm_pos, 0))
                                        error_line("借阅成功!");
                                    else    
                                        error_line("借阅失败...");
                                    break;
                                case bi_option_borrow:
                                    switch(client_shelf_moving_book(local_shelfno, &local_book, cur_dbm_pos, 1)){
                                        case 2:
                                            error_line("外借成功!");
                                            break;
                                        case 3:
                                            error_line("外借失败...");
                                            break;
                                        case 4:
                                            error_line("还书成功!");
                                            break;
                                        case 5:
                                            error_line("还书失败...");
                                            break;
                                    }
                                    break;
                                case bi_option_tagging:
                                    if(draw_bi_tagging_box(&local_book, GREET_ROW+8, WIN_WIDTH-PAD_BOXED_WIDTH)){
                                        if(client_shelf_tagging_book(local_shelfno, &local_book, cur_dbm_pos))
                                            error_line("打标签成功!");
                                        else
                                            error_line("打标签失败...");
                                    }
                                    break;
                                case bi_option_delete:
                                    if(get_confirm("即将删除?", "No")){
                                        if(client_shelf_delete_book(&local_book, cur_dbm_pos))
                                            error_line("下架图书成功!");
                                        else
                                            error_line("下架图书失败...");
                                    }
                                    break;
                                default: break; }
                            op_key = KEY_BACKSPACE;
                        } 
                    }
                    op_key = LOCAL_KEY_NON_SENSE;
                    if(local_book.encoding_time[0]){//还原现场
                        destroy_lt_option_box(&opwin);
                        clidb_book_peek(&local_book, cur_dbm_pos);
                        show_book_info(pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, &local_book, search_mode);
                    }
                }
            } 
        }   
    }

    //善后工作
    clidb_book_backward_mode();
    clear_line(NULL, pad_posx, WIN_WIDTH-PAD_BOXED_WIDTH, PAD_BOXED_HIGHT, PAD_BOXED_WIDTH);
    clear_line(NULL, pad_posx, pad_posy-3, PAD_BOXED_HIGHT, PAD_BOXED_WIDTH);
    delwin(padwin);

    //特殊处理搜索模式,返回退出键
    if(search_mode)
        if(rt_key == NON_SENSE_INT)rt_key = KEY_BACKSPACE;

    return(rt_key);
}

static int next_page_bookinfo(WINDOW *win, int *shelfno, int *bookno, book_entry_t *search_entry)
{
    book_entry_t local_book;
    int nbooks = 0, str_len = 0, str_limit = 18, res;
    char str_show[40]={0};

    //使用缓存内数据
    clidb_book_search_step(PAD_HIGHT);
    while((res = clidb_book_get(&local_book)) != -1){
        if(!res){
            error_line(local_book.name);
            touchwin(stdscr);
            refresh();
            return(-1);
        }
        str_len = strlen(local_book.name);
        strcat(str_show, "《");
        if(str_len > str_limit){
            strncpy(str_show+3, local_book.name, str_limit);
            strcat(str_show, "...》");
        }else{
            strncpy(str_show+3, local_book.name, str_len);
            strcat(str_show, "》");
        }
        mvwprintw(win, nbooks++, 0, "%s",  str_show);
        memset(str_show, '\0', sizeof(str_show));
    }

    if(nbooks > 0)return(nbooks);

    //从远端获取数据
    if(search_entry != NULL)
        res = client_searching_book(*bookno, search_entry);//搜索
    else
        res = client_shelf_loading_book(*shelfno, *bookno);//获取书架上的书籍
    if(res == 0){
        error_line("从服务器获取书籍发生异常");
        return(-1);
    }else if(res == -1){//未获取到数据
        return(0);
    }

    clidb_book_search_step(PAD_HIGHT);
    while((res = clidb_book_get(&local_book)) != -1){
        if(!res){
            error_line(local_book.name);
            touchwin(stdscr);
            refresh();
            return(-1);
        }
        str_len = strlen(local_book.name);
        strcat(str_show, "《");
        if(str_len > str_limit){
            strncpy(str_show+3, local_book.name, str_limit);
            strcat(str_show, "...》");
        }else{
            strncpy(str_show+3, local_book.name, str_len);
            strcat(str_show, "》");
        }
        mvwprintw(win, nbooks++, 0, "%s",  str_show);
        memset(str_show, '\0', sizeof(str_show));
    }

    //记录当前获取到的最大图书编号
    *bookno = local_book.code[2];

    return(nbooks);
}

static int draw_bi_tagging_box(book_entry_t *user_book, int posx, int posy)
{
    WINDOW *tagwin, *boxwin;
    int scroll_line = 0, nlines = 0, ret = 0;
    char *label_symbol = " ☘ "; 
    char local_label[(LABEL_NAME_LEN+1)*MAX_LABEL_NUM+1]="";
    char local_tag[LABEL_NAME_LEN+1];

    //外框
    boxwin = subwin(stdscr, PAD_BOXED_HIGHT-5, 27, posx-1, posy-2);
    box(boxwin, ACS_VLINE, ACS_HLINE);
    wrefresh(boxwin);

    //回卷窗口
    tagwin = newwin(PAD_BOXED_HIGHT-7, 25, posx, posy-1);
    scrollok(tagwin, TRUE);
    wrefresh(tagwin);

    //光标显示
    curs_set(1);
    cursor_state = 1;

    do{
        mvwprintw(tagwin, scroll_line, 0, "%s%02d:", label_symbol, nlines+1);
        wrefresh(tagwin);
        wgetnstr(tagwin, local_tag, LABEL_NAME_LEN);
        if(!local_tag[0])break;
        if(get_confirm("确定?", "Yes")){
            strcat(local_label, local_tag);
            strcat(local_label, ",");
            if(++scroll_line > PAD_BOXED_HIGHT-8)scroll_line=PAD_BOXED_HIGHT-8;
            nlines++;
        }else{
            if(scroll_line >= PAD_BOXED_HIGHT-8){
                scroll_line--;
                clear_line(tagwin, scroll_line, 0, 1, 26);
            }
            clear_line(tagwin, scroll_line, 0, 1, 26);
        }
    }while(nlines < 10);

    curs_set(0);
    cursor_state = 0;

    if(get_confirm("确定修改?", "Yes")){
        local_label[strlen(local_label)-1] = '\0';
        strcpy(user_book->label, local_label);
        ret = 1;
    }else{
        ret = 0;
    }

    //清屏并释放资源
    wclear(boxwin);
    wrefresh(boxwin);
    delwin(boxwin);
    wclear(tagwin);
    wrefresh(tagwin);
    delwin(tagwin);
    touchwin(stdscr);
    refresh();
    return(ret);
}

static void draw_lt_option_box(WINDOW **win, slider_t *sld, char *option[])
{
    int op_row = GREET_ROW+9;
    int op_column = WIN_WIDTH-PAD_BOXED_WIDTH;
    char **option_ptr = option;
    int i = 1, nline = 0;
    int hight = 5;

    while(*option_ptr){
        option_ptr++;
        nline++;
    }

    clear_line(NULL, op_row-1, op_column, hight+1, PAD_BOXED_WIDTH);
    hight -= 3-nline;
    op_row += 3-nline;

    *win = newwin(hight, 15, op_row, op_column);
    box(*win, ACS_VLINE, ACS_HLINE);

    option_ptr = option;
    while(*option_ptr){
        mvwprintw(*win, i, 3, *option_ptr);
        option_ptr++;
        i++;
    }

    wrefresh(*win);

    //滑块初始位置
    sld->win = stdscr;
    sld->start_posx = op_row+1;
    sld->start_posy = op_column+1;
    sld->nstep = 1;
    sld->current_row = sld->last_row = 0;
    sld->max_row = nline-1;
    move_slider(sld, 0);
}

static void destroy_lt_option_box(WINDOW **win)
{
    wclear(*win);
    wrefresh(*win);
    delwin(*win);
}

static void scroll_pad(slider_t *sld, int logic_row, int *nline)
{
    if(sld->current_row > sld->max_row && sld->last_row == 0){
        *nline = logic_row - sld->max_row;
        return;
    }
    if(sld->current_row == 0){
        *nline = 0;
        return;
    }
    if(sld->current_row > sld->max_row && sld->last_row < sld->current_row){
        *nline += 1;
        return;
    }
    if(sld->current_row >= sld->max_row && sld->last_row > sld->current_row){
        *nline -= 1;
        return;
    }
}

static void show_book_info(int startx, int starty, book_entry_t *user_book, int bShowShelf)
{ 
    char unique[8];
    char slabel[MAX_LABEL_NUM/2][2*LABEL_NAME_LEN+1];
    shelf_entry_t local_shelf;
    int str_len = 0;
    int str_limit = 20;
    char str_show[40]={0};
    int i = 0, j = 0;

    clear_line(NULL, startx, starty, PAD_BOXED_HIGHT, PAD_BOXED_WIDTH);
    memset(slabel, '\0', sizeof(slabel));

    //图书已被删除
    if(!user_book->encoding_time[0]){
        mvwprintw(stdscr, startx, starty, "本书已下架");
        return;
    }

    //计算图书唯一编码
    unique[0] = HEX_ASC[user_book->code[0]%16];
    unique[1] = HEX_ASC[user_book->code[1]/16];
    unique[2] = HEX_ASC[user_book->code[1]%16];
    unique[3] = HEX_ASC[(user_book->code[2] >> 12)&0x0F];
    unique[4] = HEX_ASC[(user_book->code[2] >> 8)&0x0F];
    unique[5] = HEX_ASC[(user_book->code[2] >> 4)&0x0F];
    unique[6] = HEX_ASC[user_book->code[2]&0x0F];
    unique[7] = '\0';

    //显示图书信息
    mvwprintw(stdscr, startx+i++, starty, "编号: %s", unique);
    str_len = strlen(user_book->author);
    if(str_len > str_limit){
        strncpy(str_show, user_book->author, str_limit);
        strcat(str_show, "...");
    }else{
        strncpy(str_show, user_book->author, str_len);
    }
    mvwprintw(stdscr, startx+i++, starty, "作者: %s",  str_show);
    memset(str_show, '\0', 40);

    if(bShowShelf){
        clidb_shelf_get(user_book->code[0], &local_shelf);
        str_len = strlen(local_shelf.name);
        if(str_len > str_limit){
            strncpy(str_show, local_shelf.name, str_limit);
            strcat(str_show, "...");
        }else{
            strncpy(str_show, local_shelf.name, str_len);
        }
        mvwprintw(stdscr, startx+i++, starty, "所属: %s", str_show);
        memset(str_show, '\0', 40);
    }

    mvwprintw(stdscr, startx+i++, starty, "位置: %d层%d排", user_book->code[1]/16, user_book->code[1]%16);
    mvwprintw(stdscr, startx+i++, starty, "上架时间: %s", user_book->encoding_time);
    if(user_book->on_reading == '1')
        mvwprintw(stdscr, startx+i++, starty, "✍    ");
    if(user_book->borrowed == '1')
        mvwprintw(stdscr, startx+i++, starty, "♞    ");
    i++;
    split_labels(user_book->label, slabel);
    while(slabel[j][0]){
        mvwprintw(stdscr, startx+i++, starty, "☘  %s", slabel[j]);
        j++;
    }

    if(user_book->encoding_time[11] == 'H')
        error_line("本书已被拾取");

    refresh();
}

static void split_labels(char *label, char rev[][37])
{
    int last_offset = -1, offset = 0;
    int index = 0;
    char ch;

    while((ch = label[offset]) != '\0'){
        if(ch == ','){
            index++;
            if(index%2 == 0){
                strncpy(rev[index/2-1], label+last_offset+1, offset-last_offset);
                rev[index/2-1][offset-last_offset-1] = ' ';
                rev[index/2-1][offset-last_offset] = 0;
                last_offset = offset;
            }
        }
        offset++;
    }
    strncpy(rev[index/2], label+last_offset+1, offset-last_offset);
} 

static int pad_fill_shelves_info(WINDOW **win, slider_t *sld, int *para_logic_row)
{
    int pad_posx = GREET_ROW+2, pad_posy = 8;
    char str_show[40] = {0};
    int str_len = 0, str_limit = 30;
    shelf_entry_t local_shelf;
    int i, nshelves = 0;

    *win = newpad(PAD_HIGHT, PAD_WIDTH);
    for(i = 1; i <= MAX_SHELF_NUM; i++){
        if(clidb_shelf_exists(i)){
            if(!clidb_shelf_get(i, &local_shelf)){
                error_line(local_shelf.name);
                return 0;
            }
            str_len = strlen(local_shelf.name);
            if(str_len > str_limit){
                strncpy(str_show, local_shelf.name, str_limit);
                strcat(str_show, "...");
            }else{
                strncpy(str_show, local_shelf.name, str_len);
            }
            mvwprintw(*win, nshelves++, 0, "%s", str_show);
            memset(str_show, '\0', sizeof(str_show));
        }
    }

    //计算实际移动的和逻辑上的最大值
    if(nshelves > PAD_BOXED_HIGHT - 1)
        sld->max_row = PAD_BOXED_HIGHT - 1;
    else
        sld->max_row = nshelves - 1;
    *para_logic_row = nshelves -1;
    sld->win = stdscr;
    sld->start_posx = pad_posx;
    sld->start_posy = pad_posy - 3;
    sld->nstep = 1;
    sld->current_row = sld->last_row = 0;

    return (1);
}

static void show_shelf_info(int startx, int starty, shelf_entry_t *user_shelf, book_count_t *user_bc)
{ 
    clear_line(NULL, startx, starty, PAD_BOXED_HIGHT, PAD_BOXED_WIDTH);
    mvwprintw(stdscr, startx, starty, "编号: %d", user_shelf->code);
    mvwprintw(stdscr, startx+1, starty, "层数: %d", user_shelf->nfloors);
    mvwprintw(stdscr, startx+2, starty, "打造时间: %s", user_shelf->building_time);
    mvwprintw(stdscr, startx+4, starty, "在架: %d", user_bc->books_all - user_bc->books_unsorted - \
            user_bc->books_on_reading - user_bc->books_borrowed);
    mvwprintw(stdscr, startx+5, starty, "在读: %d", user_bc->books_on_reading);
    mvwprintw(stdscr, startx+6, starty, "借出: %d", user_bc->books_borrowed);

    refresh();
}

static void clear_line(WINDOW* win, int startx, int starty, int nlines, int line_width)
{
    char blank_line[WIN_WIDTH]={0};

    if(win == NULL)win = stdscr;
    memset(blank_line, ' ', line_width-1);
    while(nlines--)
        mvwprintw(win, startx++, starty, blank_line);

    wrefresh(win);
}

static int get_choice(ui_page_type_e ptype, slider_t *sld, int logic_row)
{
    int key = 0;

    keypad(stdscr, TRUE);
    cbreak();
    noecho();

    while(key != KEY_ENTER && key != '\n' && key != KEY_BACKSPACE){ 
        key = getch();
        if (sld == NULL)break;
        if (key == KEY_UP){
            sld->last_row = sld->current_row;
            if( sld->current_row == 0)
                sld->current_row = logic_row;
            else
                sld->current_row--;
            move_slider(sld, 0);
            if(ptype != page_mainmenu_e)break;
        }
        if(key == KEY_DOWN){
            sld->last_row = sld->current_row;
            if(sld->current_row == logic_row)
                sld->current_row = 0;
            else
                sld->current_row++;
            move_slider(sld, 0);
            if(ptype != page_mainmenu_e)break;
        }
        if(key == KEY_LEFT || key == KEY_RIGHT)
            if(ptype != page_mainmenu_e)break;
    }

    keypad(stdscr, FALSE);
    nocbreak();
    echo();

    return(key);
}

static int get_confirm(const char *prompt, char* option_default)
{
    int confirmed = 0;
    char first_char = 'n';
    char yY_str[] = "(Y/y)";

    if(option_default[0] == 'C')//不获取用户选择,仅作为提示
        yY_str[0] = '\0';
    clear_line(NULL, Q_ROW, 1, 1, WIN_WIDTH-2);
    mvprintw(Q_ROW, 8, "%s%s (type Enter for %s)", prompt, yY_str, option_default);
    refresh();
    if(option_default[0] == 'C')//不获取用户选择,仅作为提示
        return 1;
    if(!cursor_state){
        curs_set(1);
    }

    cbreak();
    first_char = getch();
    if(first_char == 'Y' || first_char == 'y') 
        confirmed = 1;
    if(first_char == '\n'){
        if(option_default[0] == 'Y')
            confirmed = 1;
        else
            confirmed = 0;
    }
    nocbreak();

    if(!confirmed){
        clear_line(NULL, Q_ROW, 1, 1, WIN_WIDTH-2);
        mvprintw(Q_ROW, 8, "已取消");
        refresh();
        sleep(1);
    }

    clear_line(NULL, Q_ROW, 1, 1, WIN_WIDTH-2);

    //恢复光标先前状态
    if(!cursor_state){
        curs_set(0);
    }

    return confirmed;
}

static char *string_filter(char *fstr, char ch_sub)
{
    char *frptr, *fptr;
    int flen = 0;

    //过滤字符串首尾连续的空格
    fptr = fstr;
    if((flen = strlen(fstr)) > 0){
        while((*fptr == ' ' || *fptr == '\t') && flen != '\0')fptr++;
        frptr = fstr + flen - 1;
        while(*frptr == ' ' || *frptr == '\t'){
            *frptr = '\0';
            if(frptr == fstr)break;
            frptr--;
        }
        flen = strlen(fptr);
        //替换掉用户输入的制表符
        if(ch_sub){
            frptr = fptr;
            while(*frptr != '0'){
                if(*frptr == '\t')*frptr = ch_sub;
                frptr++;
            }
        }
    }

    return(fptr);
}

static int string_splitter(char *des_str, int des_len, char *src_str, int part_pos, char ch_aim)
{
    char *aptr_start, *aptr_end = src_str, *aptr_src = src_str;
    char temp_str[BOOK_NAME_LEN + AUTHOR_NAME_LEN + 32] = {0};
    int temp_pos = part_pos;

    memset(des_str, '\0', des_len+1);
    while(temp_pos > -1)
    {
        temp_pos--;
        aptr_start = aptr_end;
        if((aptr_end = strchr(aptr_src, ch_aim)) != NULL){
            aptr_src = aptr_end+1;
        }else{//未找到目标字符,直接定位到尾部
            aptr_end = src_str+strlen(src_str);
            break;
        }
    }

    if(temp_pos == -1){//找到相应子串
        if(part_pos == 0)//第0个字串特殊处理
            strncpy(temp_str, aptr_start, aptr_end-aptr_start);
        else
            strncpy(temp_str, aptr_start+1, aptr_end-aptr_start-1);
        strncpy(des_str, string_filter(temp_str, '\0'), des_len);//过滤空格字符
        return(1);
    }

    return(0);
}

//移动并显示滑块
static void move_slider(slider_t *sld, int unset)
{
    int local_last_row = sld->last_row;
    int local_current_row = sld->current_row;

    if(sld->last_row > sld->max_row)local_last_row = sld->max_row;
    if(sld->current_row > sld->max_row)local_current_row = sld->max_row;

    //取消显示上一个滑块
    mvprintw(sld->start_posx + local_last_row*sld->nstep, sld->start_posy, " ");

    //显示滑块
    mvprintw(sld->start_posx + local_current_row*sld->nstep, sld->start_posy, "❚");

    //用户指定取消显示滑块
    if(unset)mvprintw(sld->start_posx + local_current_row*sld->nstep, sld->start_posy, " ");

    wrefresh(sld->win);
}

static void error_line(char* error)
{
    clear_line(NULL, PROMPT_ROW, 1, 1, WIN_WIDTH-2);
    mvwprintw(stdscr, PROMPT_ROW, (WIN_WIDTH-strlen(error)/3*2)/2, error);
    refresh();
}

