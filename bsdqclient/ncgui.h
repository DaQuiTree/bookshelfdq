#ifndef _NCGUI_H_
#define _NCGUI_H_

#include <ncurses.h>
#include "clisrv.h"

typedef enum{
    lt_option_open,
    lt_option_destroy,
    lt_option_newbook
}ui_lookthrough_option_e;

typedef enum{
    bi_option_tagging,
    bi_option_delete,
}ui_bookinfo_option_e;

typedef enum{
    page_nonsense_e,
    page_mainmenu_e,
    page_lookthrough_e,
    page_bookinfo_e
}ui_page_type_e;

typedef enum{
    menu_look_through_e,
    menu_build_shelf_e,
    menu_search_book_e,
    menu_quit_e,
}ui_menu_e;

typedef struct{
    WINDOW *win;
    int start_posx;
    int start_posy;
    int nstep;
    int current_row;
    int last_row;
    int max_row;//滑块最多可移动行
}slider_t;

int client_start_gui(char* hostname, char *user);

void ncgui_init(void);
void ncgui_close(void);
void ncgui_clear_all_screen(void);

ui_menu_e ncgui_get_choice(void);
void ncgui_display_mainmenu_page(void);
void ncgui_display_lookthrough_page(void);

#endif
