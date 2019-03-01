#include <stdio.h>
#include <stdlib.h>
#include "ncgui.h"

int main()
{
    int running = 1;
    int ret_menu = menu_non_sense_e, op_menu = menu_non_sense_e;
    
    ncgui_init();

    if(ncgui_display_login_page() == menu_quit_e){
        ncgui_close();
        exit(EXIT_SUCCESS);
    }//用户选择退出登录

    if(!ncgui_sync_from_server())
        exit(EXIT_FAILURE);
    while(running){
        if(ret_menu != menu_cycle_e){//程序自身有循环刷新当前界面要求
            ncgui_clear_all_screen();
            ncgui_display_mainmenu_page();
            op_menu = ncgui_get_choice();
        }
        switch(op_menu)
        {
            case menu_look_through_e:
                ncgui_clear_all_screen();
                ret_menu = ncgui_display_lookthrough_page();
                break;
            case menu_build_shelf_e:
                ncgui_clear_all_screen();
                ret_menu = ncgui_display_buildshelf_page();
                break;
            case menu_search_book_e:
                ncgui_clear_all_screen();
                ret_menu = ncgui_display_searchbook_page();
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
