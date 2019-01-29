#ifndef _CLISRV_H_
#define _CLISRV_H_


//书相关定义
#define MAX_BOOK_NUM (10000)
#define BOOK_NAME_LEN (64*3) //UTF-8中文占用3个字节 
#define AUTHOR_NAME_LEN (32*3)
#define MAX_LABEL_NUM (10)
#define LABEL_NAME_LEN (6*3)

typedef {
    int  code[4];
    char name[BOOK_NAME_LEN+1];
    char author[AUTHER_NAME_LEN+1];
    char label[MAX_LABEL_NUM*LABEL_NAME_LEN+1]; 
    int  borrowed;
    int  on_reading;
    char encoding_time[64];
}book_entry_t;

//书架相关定义
#define MAX_SHELF_NUM (0x10)
#define SHELF_NAME_LEN (64*3)
#define MAX_FLOORS (10)
#define MAX_DEPTH (5)

typedef {
    int code;
    char name[SHELF_NAME_LEN+1];
    int floors_of_shelf;//书架层数
    int depth_of_floor[MAX_DEPTH];//层深度
    char building_time[64]
}shelf_entry_t;

//socket通讯相关
#define USER_NAME_LEN (64) 
#define ERR_TEXT_LEN (80)

typedef union {
    shelf_entry_t shelf;
    book_entry_t book;
}trans_stuff_un;

typedef enum{
}client_request_e;

typedef enum{
    r_success;
    r_failed;
    r_findnomore;
}server_response_e;

typedef struct {
    char                user[USER_NAME_LEN+1];
    clent_request_e     request;
    trans_stuff_un      stuff;
    server_response_e   response;
    char                error_text[ERR_TEXT_LEN+1];
}message_cs_t; 

//检索相关
#define DEFAULT_FINDS (10)
#define NON_SENSE_INT (0)
#define NON_SENSE_STR ('\0')
#define FIND_FLAG_INT (-32700)

//全局定义
#define BOOK_AVL (0)
#define BOOK_DEL (1)
#define BOOK_UNDEF (2)

#endif

