#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "socketcom.h"
#include "clishell.h"
#include "clisrv.h"
#include "clidb.h"

char login_user[USER_NAME_LEN+1];

static int find_from_server(message_cs_t *msg);

//
//封装client端初始化操作
//

int client_start_login(char*host, account_entry_t *user_account, char *errInfo)
{
    //socket连接
    if (!socket_client_init(host)){
#if DEBUG_TRACE
        fprintf(stderr, "client socket initialize failed.\n");
#endif
        strcpy(errInfo, "无法连接服务器...");
        return(0);
    }
    
    if(user_account->type == account_user_e){
        //验证普通登录身份
        if (!client_verify_account(user_account, 1, errInfo)){
            socket_client_close();
            return(0);
        }
        if (!clidb_init(user_account->name)){//dbm数据库初始化
#if DEBUG_TRACE
            fprintf(stderr, "client database initialize failed.\n");
#endif
            strcpy(errInfo, "dbm初始化失败...");
            socket_client_close();
            return(0);
        }
        strcpy(login_user, user_account->name);
    }else if(user_account->type == account_admin_e){
        //验证管理员身份
        if (!client_verify_account(user_account, 0, errInfo)){
            socket_client_close();
            return(0);
        }
    }

    return(1);
}

int client_start_register(char *host, account_entry_t *user_account, char *errInfo)
{
    //socket连接
    if (!socket_client_init(host)){
#if DEBUG_TRACE
        fprintf(stderr, "client socket initialize failed.\n");
#endif
        strcpy(errInfo, "无法连接服务器...");
        return(0);
    }
    
    //验证用户身份
    if (!client_register_account(user_account, errInfo))return(0);

    //断开与server连接
    socket_client_close();

    return(1);
}

int client_books_count_sync(book_count_t *bc)
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
        *bc = *(book_count_t *)&msgget.extra_info;
#if DEBUG_TRACE
        printf("all: %d, borrowed: %d, on_reading: %d, unsorted: %d\n",\
                bc->books_all, bc->books_borrowed, bc->books_on_reading, bc->books_unsorted);
#endif
        return(1);
    }
#if DEBUG_TRACE
    fprintf(stderr, "books_sync error: client get response error.\n");
#endif
    return(0);
}

int client_shelves_count_sync(shelf_count_t *sc)
{
    message_cs_t msgreq, msgget;
    int res;

    strcpy(msgreq.user, login_user);
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
        *sc = *(shelf_count_t *)&msgget.extra_info;
#if DEBUG_TRACE
        printf("shelf all: %d\n", sc->shelves_all);
#endif
    }else if(msgget.response == r_failed){
#if DEBUG_TRACE
        fprintf(stderr, "shelves_sync error: some error occered on server when get shelf num %s.\n",\
                msgget.error_text);
#endif
    }

    return (1);
}

int client_shelves_info_sync(shelf_count_t *sc)
{
    message_cs_t msgreq, msgget;
    int res, i;
    int shelf_cnt = 0;

    strcpy(msgreq.user, login_user);

    //获取书架总数
    if(!client_shelves_count_sync(sc))return (0);

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

    if(shelf_cnt != sc->shelves_all){
#if DEBUG_TRACE
        fprintf(stderr, "shelves_sync error: shelf sync num not equal to remote shelf num.\n");
#endif
        return(0);
    }

    return(1);
}


//
//封装client端的socket请求
//

int client_shelf_insert_book(book_entry_t *user_book, char *errInfo)
{
    message_cs_t msg;
    int res;
    struct tm *tm_ptr;
    time_t the_time;

    memset(&msg, '\0', sizeof(message_cs_t));
    //信息不完整
    if((user_book->name[0] && user_book->author[0] && user_book->code[1]) == 0)return 0;

    //记录时间
    (void)time(&the_time);
    tm_ptr = localtime(&the_time);
    sprintf(user_book->encoding_time, "%02d-%02d-%02d %02dh",\
            tm_ptr->tm_year-100, tm_ptr->tm_mon+1, tm_ptr->tm_mday, tm_ptr->tm_hour);

    strcpy(msg.user, login_user);
    msg.request = req_insert_book_e;
    msg.stuff.book = *user_book; msg.stuff.book.code[2] = NON_SENSE_INT;

    res = find_from_server(&msg);
    if(res == 1)
        clidb_book_insert(&msg.stuff.book, -1);
    else
        strcpy(errInfo, msg.error_text);
    return(res);
}

int client_shelf_loading_book(int shelfno, int bookno)
{
    message_cs_t msg;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_find_book_e;

    msg.stuff.book.code[0] = shelfno;
    msg.stuff.book.code[1] = NON_SENSE_INT;
    msg.stuff.book.code[2] = bookno;
    strcpy(msg.stuff.book.name, "-");
    strcpy(msg.stuff.book.author, "-");
    strcpy(msg.stuff.book.label, "-");
    msg.stuff.book.borrowed = '0';
    msg.stuff.book.on_reading = '0';

    if(shelfno == NON_SENSE_INT){
        msg.stuff.book.borrowed = FLAG_FIND_UNSORTED;
        msg.stuff.book.on_reading = FLAG_FIND_UNSORTED;
    }

    return(find_from_server(&msg));
}

int client_shelf_delete_book(book_entry_t *user_book, int dbm_pos)
{
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_delete_book_e;
    msg.stuff.book.code[0] = NON_SENSE_INT;
    msg.stuff.book.code[1] = NON_SENSE_INT;
    msg.stuff.book.code[2] = user_book->code[2];

    res = find_from_server(&msg);
    if(res == 1){
        clidb_book_delete(dbm_pos);
        user_book->encoding_time[0] = '\0';
    }

    return(res);
}

int client_shelf_collecting_book(int shelfno, book_entry_t *user_book, int dbm_pos)
{
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_update_book_e;
    msg.stuff.book = *user_book;
    msg.stuff.book.code[0] = shelfno;
    res = find_from_server(&msg);
    if(res == 1){
        user_book->encoding_time[11] = 'H';
        clidb_book_insert(user_book, dbm_pos);
    }

    return(res);
}

int client_shelf_tagging_book(int shelfno, book_entry_t *user_book, int dbm_pos)
{
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_update_book_e;
    msg.stuff.book = *user_book;
    res = find_from_server(&msg);

    if(res == 1)
        clidb_book_insert(user_book, dbm_pos);

    return(res);
}

int client_shelf_count_book(int shelfno, book_count_t *user_bc)
{
    message_cs_t msg;
    int res = 0;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.stuff.book.code[0] = shelfno;
    msg.stuff.book.code[1] = NON_SENSE_INT;
    msg.request = req_count_book_e;

    res = find_from_server(&msg);

    if(res == 1)
        *user_bc = *(book_count_t *)&msg.extra_info;

    return(res);
}

int client_shelf_moving_book(int shelfno, book_entry_t *user_book, int dbm_pos, int move_action)
{
    message_cs_t msg;
    int res, ret = 0;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_update_book_e;
    msg.stuff.book = *user_book;
    if(move_action == 0){//读书需求
        msg.stuff.book.on_reading = '1';
        ret = 1;
    }else if(move_action == 1){//还书或外借需求
        //是否应该还书?
        if(msg.stuff.book.on_reading == '1'){
            msg.stuff.book.on_reading = '0';
            ret = 4;
        }else if(msg.stuff.book.borrowed == '1'){
            msg.stuff.book.borrowed = '0';
            ret = 4;
        }else{//外借
            msg.stuff.book.borrowed = '1';
            ret = 2;
        }
    }
    res = find_from_server(&msg);

    if(res == 1)
        clidb_book_insert(&msg.stuff.book, dbm_pos);
    else
        ret++;

    return(ret);
}

int client_shelf_abandon_books(int shelfno)
{
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_delete_book_e;
    msg.stuff.book.code[0] = shelfno;
    msg.stuff.book.code[2] = NON_SENSE_INT;
    res = find_from_server(&msg);

    return(res);
}

int client_shelf_unsorted_books(int shelfno)
{
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_update_book_e;
    memset(&msg.stuff.book, '\0', sizeof(book_entry_t));
    msg.stuff.book.code[0] = shelfno;
    msg.stuff.book.code[2] = NON_SENSE_INT;
    res = find_from_server(&msg);

    return(res);
}

int client_shelf_delete_itself(int shelfno)
{
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_remove_shelf_e;
    memset(&msg.stuff.shelf, '\0', sizeof(shelf_entry_t));
    msg.stuff.shelf.code = shelfno;
    res = find_from_server(&msg);

    if(res == 1)
        (void)clidb_shelf_delete(shelfno);

    return(res);
}

int client_searching_book(int bookno, book_entry_t *search_entry)
{
    static int new_mark = 0;//使用encoding_time保存检索结果,并使用第一个字符作为新检索标志
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_find_book_e;

    msg.stuff.book = *search_entry;
    msg.stuff.book.code[1] = NON_SENSE_INT;
    msg.stuff.book.code[2] = bookno;

    res = find_from_server(&msg);

    if(bookno == BREAK_LIMIT_INT){
        new_mark ^= 1;
        sprintf(search_entry->encoding_time, "%d 检索到: %d 册", new_mark, msg.extra_info[0]);
    }

    return(res);
}

int client_build_shelf(shelf_entry_t *user_shelf, char *errInfo)
{
    message_cs_t msg;
    int res;
    struct tm *tm_ptr;
    time_t the_time;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, login_user);
    msg.request = req_build_shelf_e;
    msg.stuff.shelf = *user_shelf;

    //记录时间
    (void)time(&the_time);
    tm_ptr = localtime(&the_time);
    sprintf(msg.stuff.shelf.building_time, "%02d-%02d-%02d %02dh",\
            tm_ptr->tm_year-100, tm_ptr->tm_mon+1, tm_ptr->tm_mday, tm_ptr->tm_hour);

    res = find_from_server(&msg);

    if(res == 1)
        clidb_shelf_insert(&msg.stuff.shelf);
    else
        strcpy(errInfo, msg.error_text);
    return(res);
}

int client_register_account(account_entry_t *user_account, char *errInfo)
{
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, user_account->name);
    msg.request = req_register_account_e;
    msg.stuff.account = *user_account;
    res = find_from_server(&msg);
    if(res == 0){
        if(msg.error_text[0] == '\0')
            strcpy(errInfo, "注册账户失败");
        else
            strcpy(errInfo, msg.error_text);
    }

    return(res);
}

int client_verify_account(account_entry_t *user_account, int init_mode, char *errInfo)
{
    message_cs_t msg;
    int res;

    memset(&msg, '\0', sizeof(message_cs_t));
    strcpy(msg.user, user_account->name);
    msg.request = req_verify_account_e;
    msg.stuff.account = *user_account;
    if(init_mode == 1 && user_account->type == account_user_e)
        msg.extra_info[0] = 1;
    res = find_from_server(&msg);

    if(res == 0){
        if(msg.error_text[0] == '\0')
            strcpy(errInfo, "账户验证失败");
        else
            strcpy(errInfo, msg.error_text);
    }

    return(res);
}

static int find_from_server(message_cs_t *msg)
{
    int res;
    int nfinds = 0;
    int flag_not_cached = 0;

    if(msg->request == req_find_book_e && msg->stuff.book.code[2] == BREAK_LIMIT_INT)
        flag_not_cached = 1;
    res = socket_client_send_request(msg);
    if(!res)return(0);

    do{
        res = socket_client_get_response(msg);
        if(!res)return(0);

        //检查结果
        if(msg->response == r_failed){
            return(0);
        }else if(msg->response == r_find_end){
#if DEBUG_TRACE
            printf("Client find no more.\n");
#endif
            if(nfinds == 0)return(-1);//没有获取到任何数据
            return(1);
        }else if(msg->response == r_find_end_more){
#if DEBUG_TRACE
            printf("client find more.\n");
#endif
            return(1);
        }else{
            if(msg->request == req_find_book_e){//搜索命令特殊处理
                if(!flag_not_cached)
                    clidb_book_insert(&msg->stuff.book, -1);
                nfinds++;
            }else{
                return(1);//执行成功
            }   
        }
    }while(1);

    return(0);
}

