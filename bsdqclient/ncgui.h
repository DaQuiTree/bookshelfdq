#ifndef _NCGUI_H_
#define _NCGUI_H_

#include <ncurses.h>
#include "clisrv.h"

typedef enum{
    ds_option_collect,
    ds_option_abandon
}ui_shelfdel_option_e;

typedef enum{
    lt_option_open,
    lt_option_newbook,
    lt_option_profile,
    lt_option_destroy,
    lt_option_collectbook,
}ui_lookthrough_option_e;

typedef enum{
    sb_option_select_shelf,
    sb_option_finish
}ui_searchbook_option_e;

typedef enum{
    bi_option_reading,
    bi_option_borrow,
    bi_option_tagging,
    bi_option_delete
}ui_bookinfo_option_e;

typedef enum{
    nb_option_name,
    nb_option_author,
    nb_option_nfloor,
    nb_option_tagging,
    nb_option_finish
}ui_newbook_option_e;

typedef enum{
    page_nonsense_e,
    page_mainmenu_e,
    page_lookthrough_e,
    page_buildshelf_e,
    page_searchbook_e,
    page_bookinfo_e,
    page_newbook_e,
}ui_page_type_e;

typedef enum{
    menu_look_through_e,
    menu_build_shelf_e,
    menu_search_book_e,
    menu_quit_e,
    menu_non_sense_e,
    menu_cycle_e
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

int ncgui_connect_to_server(char* hostname, char *user);
int ncgui_sync_from_server(void);

void ncgui_init(void);
void ncgui_close(void);
void ncgui_clear_all_screen(void);

ui_menu_e ncgui_get_choice(void);
void ncgui_display_mainmenu_page(void);
ui_menu_e ncgui_display_lookthrough_page(void);
ui_menu_e ncgui_display_buildshelf_page(void);
ui_menu_e ncgui_display_searchbook_page(void);

#endif
