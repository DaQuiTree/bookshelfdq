#ifndef _CLISRV_H_
#define _CLISRV_H_

//书相关定义
#define MAX_BOOK_NUM (36)
#define BOOK_NAME_LEN (64*3) //UTF-8中文占用3个字节 
#define AUTHOR_NAME_LEN (32*3)
#define MAX_LABEL_NUM (10)
#define LABEL_NAME_LEN (6*3)

typedef struct{
    int  code[3];
    char name[BOOK_NAME_LEN+1];
    char author[AUTHOR_NAME_LEN+1];
    char label[MAX_LABEL_NUM*(LABEL_NAME_LEN+1)+1]; 
    char borrowed;
    char on_reading;
    char encoding_time[20];
}book_entry_t;

typedef struct{
    int books_all;
    int books_on_reading;
    int books_borrowed;
    int books_unsorted;
}book_count_t;

//书架相关定义
#define MAX_SHELF_NUM (0x0F)
#define SHELF_NAME_LEN (64*3)
#define MAX_FLOORS (0x0F)
#define MAX_DEPTH (5)

typedef struct{
    int code;
    char name[SHELF_NAME_LEN+1];
    unsigned char nfloors;//书架层数
    unsigned char ndepth[MAX_FLOORS];//层深度
    char building_time[20];
}shelf_entry_t;

typedef struct{
    int shelves_all;
}shelf_count_t;

//socket通讯相关
#define USER_NAME_LEN (64) 
#define ERR_TEXT_LEN (80)
#define EXTRA_INFO_SIZ (4)

typedef union{
    shelf_entry_t shelf;
    book_entry_t book;
}trans_stuff_un;

typedef enum{
    req_insert_book_e,
    req_delete_book_e,
    req_update_book_e,
    req_find_book_e,
    req_count_book_e,
    req_build_shelf_e,
    req_remove_shelf_e,
    req_find_shelf_e,
    req_count_shelf_e
}client_request_e;

typedef enum{
    r_success,
    r_failed,
    r_find_end_more,
    r_find_end
}server_response_e;

typedef struct{
    char                user[USER_NAME_LEN+1];
    client_request_e    request;
    trans_stuff_un      stuff;
    server_response_e   response;
    char                error_text[ERR_TEXT_LEN+1];
    void*               extra_info[EXTRA_INFO_SIZ];
}message_cs_t; 


//检索相关
#define DEFAULT_FINDS (30)

#define NON_SENSE_INT (0)
#define BREAK_LIMIT_INT (MAX_BOOK_NUM + 1)

#define FLAG_BORROWED (1)
#define FLAG_ON_READING (1)
#define FLAG_FIND_UNSORTED (2)

#define FETCH_RESULT_END (3)
#define FETCH_RESULT_END_MORE (2)
#define FETCH_RESULT_CONT (1)
#define FETCH_RESULT_ERR (0)


//全局定义
#define BOOK_AVL (0)
#define BOOK_DEL (1)
#define BOOK_UNSORTED (2)

#define SHELF_AVL (0)
#define SHELF_DEL (1)

#endif

