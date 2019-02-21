#include <stdio.h>
#include <stdlib.h>
#include "ncgui.h"

int main()
{
    int running = 1;
    int ret_menu = menu_non_sense_e, op_menu = menu_non_sense_e;
    
    if(!client_start_gui("127.0.0.1", "daqui"))
        exit(EXIT_FAILURE);

    ncgui_init();
    while(running){
        ncgui_clear_all_screen();
        ncgui_display_mainmenu_page();
        if(ret_menu != menu_cycle_e)//程序自身有循环刷新当前界面要求
            op_menu = ncgui_get_choice();
        else
            op_menu = ret_menu;
        switch(op_menu)
        {
            case menu_look_through_e:
                ncgui_clear_all_screen();
                ret_menu = ncgui_display_lookthrough_page();
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
