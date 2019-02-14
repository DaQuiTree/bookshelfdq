#include <stdio.h>
#include <stdlib.h>
#include "ncgui.h"

int main()
{
    int running = 1;
    
    if(!client_start_gui("127.0.0.1", "daqui"))
        exit(EXIT_FAILURE);

    ncgui_init();
    while(running){
        ncgui_clear_all_screen();
        ncgui_display_mainmenu_page();
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

    exit(EXIT_SUCCESS);
}
