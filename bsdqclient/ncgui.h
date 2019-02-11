#ifndef _NCGUI_H_
#define _NCGUI_H_

#include "clisrv.h"

void ncgui_init(char* user);
void ncgui_close(void);
//char ncgui_get_choice(void);
void ncgui_draw_mainmenu(int hl_row, shelf_count_t sc, book_count_t bc);

#endif
