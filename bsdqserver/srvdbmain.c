#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "srvdb.h"
#include "clisrv.h"

void show_result(void);

int main()
{
    int res, rows;
    message_cs_t msg;

    strcpy(msg.user, "daqui");

    msg.stuff.book.borrowed = 0;
    msg.stuff.book.on_reading = 0;
    msg.stuff.book.code[0] = 2;
    msg.stuff.book.code[1] = NON_SENSE_INT;
    msg.stuff.book.code[2] = 15;

    strcpy(msg.stuff.book.name, "0");
    strcpy(msg.stuff.book.author, "0");
    strcpy(msg.stuff.book.label, "0");
    //strcpy(msg.stuff.book.encoding_time, "20190131");
    res = srvdb_init();
    if(!res)return(EXIT_FAILURE);
    printf("init successed\n");
    res = srvdb_connect("127.0.0.1", 0);
    if(!res)return(EXIT_FAILURE);
    printf("connect successed\n");
    res = srvdb_user_archive_init("daqui");
    if(!res)return(EXIT_FAILURE);
    printf("archive init successed\n");
    res = srvdb_book_find(&msg, &rows);
    if(!res)return(EXIT_FAILURE);
    printf("find %d rows\n", rows);
    show_result();
}

void show_result(void)
{
    message_cs_t msg;

    while(srvdb_book_fetch_result(&msg)){
        printf("%d %s %s %s %s\n", msg.stuff.book.code[2],\
                msg.stuff.book.name, msg.stuff.book.author, msg.stuff.book.label, msg.stuff.book.encoding_time);
    }

    srvdb_free_result();
}

//update 
    /*res = srvdb_book_update(&msg);*/
    /*if(!res)return(EXIT_FAILURE);*/
    /*printf("update 11 success.\n");*/
    /*msg.stuff.book.code[0] = 2;*/
    /*msg.stuff.book.code[1] = 0x31;*/
    /*msg.stuff.book.code[2] = 1;*/
    /*strcpy(msg.stuff.book.name, "约翰.克里斯朵夫");*/
    /*strcpy(msg.stuff.book.author, "罗曼.罗兰著 傅雷译");*/
    /*strcpy(msg.stuff.book.label, "青年奋斗, 文学巨著, 傅氏神译");*/
    /*msg.stuff.book.borrowed = 0;*/
    /*msg.stuff.book.on_reading = 1;*/
    /*res = srvdb_book_update(&msg);*/
    /*if(!res)return(EXIT_FAILURE);*/
    /*printf("update 1 success.\n");*/


//insert
    //res = srvdb_book_insert(&msg);
    /*printf("insert successed\n");*/
/*    msg.stuff.book.code[2] = 3;*/

//delete
    /*res = srvdb_book_delete(&msg);*/
    /*if(!res)return(EXIT_FAILURE);*/
    /*msg.stuff.book.code[2] = 10;*/
    /*res = srvdb_book_delete(&msg);*/
    /*if(!res)return(EXIT_FAILURE);*/
    /*msg.stuff.book.code[2] = 11;*/
    /*res = srvdb_book_delete(&msg);*/
    /*if(!res)return(EXIT_FAILURE);*/
    /*printf("group delete successed\n");*/
/*}*/

//find
    /*msg.stuff.book.borrowed = 0;*/
    /*msg.stuff.book.on_reading = 0;*/
    /*msg.stuff.book.code[0] = 1;*/
    /*msg.stuff.book.code[1] = 0x11;*/
    /*msg.stuff.book.code[2] = NON_SENSE_INT;*/

    /*strcpy(msg.stuff.book.name, "0");*/
    /*strcpy(msg.stuff.book.author, "0");*/
    /*strcpy(msg.stuff.book.label, "+经");*/
    /*strcpy(msg.stuff.book.encoding_time, "20190131");*/
