#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clidb.h"
#include "clisrv.h"

static void showresult(book_entry_t *user_book);
static void getbook(book_entry_t *user_book, char *book_name);

int main()
{
    book_entry_t book, newbook;
    int result, i = 1;

    if(!clidb_init("daqui")){
        exit(EXIT_FAILURE);
    }
    printf("init ok!\n");

    getbook(&book, "三国演义");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "三国");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "三毛流浪记");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "三个和尚");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }

    for(i = 0; i < 15; i++){
        result = clidb_book_backward_get(&newbook);
        if(result == 1){
            showresult(&newbook);
        }else if(result == -1){
            printf("backward no more.\n");
        }else{
            printf("error");
            exit(1);
        }
    }

    clidb_book_reset();

    getbook(&book, "拉开建立空间");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "离开立空");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "你们，你");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "了科技哦");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "哦iu");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "三国dawjjj演义");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "l;kjiojh");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "我耳机");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "去玩儿");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }
    getbook(&book, "阿斯顿服务额");
    if(clidb_book_insert(&book)){
        printf("%d insert ok\n", i++);
    }

    for(i = 0; i < 15; i++){
        result = clidb_book_forward_get(&newbook);
        if(result == 1){
            showresult(&newbook);
        }else if(result == -1){
            printf("forward no more.\n");
        }else{
            printf("error");
            exit(1);
        }
    }
}

static void getbook(book_entry_t *user_book, char *book_name)
{
    static int num = 1;

    user_book->code[0] = 1;
    user_book->code[1] = 0x21;
    user_book->code[2] = num++;
    strcpy(user_book->name, book_name);
    strcpy(user_book->author, "匿名");
    strcpy(user_book->label, "无 无无 无无无 无无无无");
    user_book->borrowed = 0;
    user_book->on_reading = 0;
    strcpy(user_book->encoding_time, "20190208下午");
}

static void showresult(book_entry_t *user_book)
{
    printf("%d\n", user_book->code[2]);
    printf("%s\n", user_book->name);
    printf("%s\n", user_book->author);
    printf("%s\n", user_book->label);
    printf("%s\n", user_book->encoding_time);
}
