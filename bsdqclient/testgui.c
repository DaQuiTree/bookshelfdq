#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "clisrv.h"
#include "ncgui.c"

int main()
{
    shelf_count_t sct;
    book_count_t bct;

    sct.shelves_all = 10;
    bct.books_all = 10000;
    bct.books_on_reading = 20;
    bct.books_borrowed = 15;
    bct.books_unsorted = 150;

    ncgui_init("daquijerry");
    ncgui_draw_mainmenu(1, sct, bct);
    getch();
    ncgui_close();
}
