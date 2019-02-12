#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socketcom.h"
#include "clishell.h"
#include "clisrv.h"
#include "clidb.h"
#include "ncgui.h"

char login_user[USER_NAME_LEN+1];
book_count_t bc;
shelf_count_t sc;

int client_initialize(char* host, char* user)
{
    if (!clidb_init(user)){
#if DEBUG_TRACE
        fprintf(stderr, "client database initialize failed.\n");
#endif
        return(0);
    }

    if (!socket_client_init(host)){
#if DEBUG_TRACE
        fprintf(stderr, "client socket initialize failed.\n");
#endif
        return(0);
    }
    
    strcpy(login_user, user);
    return(1);
}

int client_shelves_info_sync(void)
{
    message_cs_t msgreq, msgget;
    int res, i;
    int shelf_cnt = 0;

    strcpy(msgreq.user, login_user);

    //获取书架总数
    msgreq.request = req_count_shelf_e;
    res = socket_client_send_request(&msgreq);
    if(!res){
#if DEBUG_TRACE
        fprintf(stderr, "shelves_sync error: client send request error when get shelf num.\n");
#endif
        return(0);
    }

    res = socket_client_get_response(&msgget);
    if(!res){
#if DEBUG_TRACE
        fprintf(stderr, "shelves_sync error: client get response error when get shelf num.\n");
#endif
        return(0);
    }
    if(msgget.response == r_success){
        sc = *(shelf_count_t *)&msgget.extra_info;
#if DEBUG_TRACE
        printf("shelf all: %d\n", sc.shelves_all);
#endif
    }else if(msgget.response == r_failed){
#if DEBUG_TRACE
        fprintf(stderr, "shelves_sync error: some error occered on server when get shelf num %s.\n",\
                msgget.error_text);
#endif
    }

    //远程同步
    msgreq.request = req_find_shelf_e;
    msgreq.stuff.shelf.code = NON_SENSE_INT; //查找全部书架信息
    res = socket_client_send_request(&msgreq);
    if(!res){
#if DEBUG_TRACE
        fprintf(stderr, "shelves_sync error: client send sync request error.\n");
#endif
        return(0);
    }

    do{
        res = socket_client_get_response(&msgget);
        if(!res){
#if DEBUG_TRACE
            fprintf(stderr, "shelves_sync error: client get sync response error.\n");
#endif
            return(0);
        }
        
        switch(msgget.response)
        {
            case r_success:
                res = clidb_shelf_synchronize(&msgget.stuff.shelf);
                if(!res){
#if DEBUG_TRACE
                    fprintf(stderr, "shelves_sync error: shelf synchronize error.\n");
#endif                    
                    return(0);
                }
                shelf_cnt++;
                break;
            case r_find_end_more:
                msgreq.stuff.shelf.code = msgget.stuff.shelf.code;
                res = socket_client_send_request(&msgreq);
                if(!res){
#if DEBUG_TRACE
                    fprintf(stderr, "shelves_sync error: client send sync request error.\n");
#endif
                    return(0);
                }
                break;
            case r_failed:
#if DEBUG_TRACE
                fprintf(stderr, "shelves_sync error: some error occred on server %s.\n", msgget.error_text);
#endif
                break;
            default: break;
        }
    }while(msgget.response != r_find_end);

    //未同步数据再次请求
    for(i = 1; i <= MAX_SHELF_NUM; i++){
        if(clidb_shelf_not_syncs(i)){
            msgreq.stuff.shelf.code = -i; //查找指定书架信息
            res = socket_client_send_request(&msgreq);
            if(!res){
#if DEBUG_TRACE
                fprintf(stderr, "shelves_sync error: client send pointed sync request error.\n");
#endif
                return(0);
            }
            res = socket_client_get_response(&msgget);
            if(!res){
#if DEBUG_TRACE
                fprintf(stderr, "shelves_sync error: client get pointed sync response error.\n");
#endif
                return(0);
            }
            switch(msgget.response)
            {
                case r_success:
                    res = clidb_shelf_synchronize(&msgget.stuff.shelf);
                    if(!res){
#if DEBUG_TRACE
                        fprintf(stderr, "shelves_sync error: shelf pointed synchronize error.\n");
#endif
                        return(0);
                    }
                    shelf_cnt++;
                    break;
                case r_find_end:
                    res = clidb_shelf_delete(i);
                    if(!res){
#if DEBUG_TRACE
                        fprintf(stderr, "shelves_sync error: delete shelf error.\n");
#endif
                        return(0);
                    }
                    break;
                case r_failed:
#if DEBUG_TRACE
                    fprintf(stderr, "shelves_sync error: some error occred on server %s.\n", msgget.error_text);
#endif
                    break;
                default: break;
            }
        }
    }

    //重新更新书架记录
    res = clidb_shelf_record_sort();
    if(!res){
#if DEBUG_TRACE
        fprintf(stderr, "shelves_sync error: sort shelf error.\n");
#endif
        return(0);
    }

    if(shelf_cnt != sc.shelves_all){
#if DEBUG_TRACE
        fprintf(stderr, "shelves_sync error: shelf sync num not equal to remote shelf num.\n");
#endif
        return(0);
    }

    return(1);
}

int client_books_info_sync(void)
{
    message_cs_t msgreq, msgget;
    int res;

    strcpy(msgreq.user, login_user);
    msgreq.stuff.book.code[0] = NON_SENSE_INT;
    msgreq.stuff.book.code[1] = NON_SENSE_INT;
    msgreq.request = req_count_book_e;

    res = socket_client_send_request(&msgreq);
    if(!res){
#if DEBUG_TRACE
        fprintf(stderr, "books_sync error: client send request error.\n");
#endif
        return(0);
    }
    
    res = socket_client_get_response(&msgget);
    if(!res){
#if DEBUG_TRACE
        fprintf(stderr, "books_sync error: client get response error.\n");
#endif
        return(0);
    }

    if(msgget.response == r_success){
        bc = *(book_count_t *)&msgget.extra_info;
#if DEBUG_TRACE
        printf("all: %d, borrowed: %d, on_reading: %d, unsorted: %d\n",\
                bc.books_all, bc.books_borrowed, bc.books_on_reading, bc.books_unsorted);
#endif
        return(1);
    }
#if DEBUG_TRACE
    fprintf(stderr, "books_sync error: client get response error.\n");
#endif
    return(0);
}

int client_start_gui(void)
{
    int running = 1;
    
    ncgui_init(login_user);
    while(running){
        ncgui_display_mainmenu_page(sc, bc);
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

    return(1);
}
