#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "clisrv.h"
#include "ncgui.h"

int main()
{
    int running = 1;
    shelf_count_t sct;
    book_count_t bct;

    sct.shelves_all = 10;
    bct.books_all = 10000;
    bct.books_on_reading = 20;
    bct.books_borrowed = 15;
    bct.books_unsorted = 150;

    ncgui_init("daquijerry");
    ncgui_display_mainmenu_page(sct, bct);
    while(running){
        switch(ncgui_get_choice())
        {
            case menu_look_through_e:
                ncgui_clear_all_screen();
                ncgui_display_lookthrough_page();
                break;
            case menu_quit_e:
                running = 0;
                break;
            default: break;
        }
    }
    ncgui_close();
}
