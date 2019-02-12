#ifndef _NCGUI_H_
#define _NCGUI_H_

#include "clisrv.h"

typedef enum{
    menu_look_through_e,
    menu_build_shelf_e,
    menu_search_book_e,
    menu_quit_e,
}ui_menu_e;

typedef struct{
    int start_posx;
    int start_posy;
    int nstep;
    int current_row;
    int last_row;
}slider_t;

void ncgui_init(char* user);
void ncgui_close(void);
void ncgui_clear_all_screen(void);

ui_menu_e ncgui_get_choice(void);
void ncgui_display_mainmenu_page(shelf_count_t sc, book_count_t bc);
void ncgui_display_lookthrough_page(void);

#endif
