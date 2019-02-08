#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clisrv.h"
#include "socketcom.h"

void insert_message(message_cs_t *msg);
void delete_message(message_cs_t *msg);
void update_message(message_cs_t *msg);
void find_message(message_cs_t *msg);
void count_message(message_cs_t *msg);
void nextstep(message_cs_t *msg);

int main(void)
{
    message_cs_t msg;

    if(!socket_client_init("127.0.0.1"))
    {
        fprintf(stderr, "client init error.\n");
        exit(EXIT_FAILURE);
    }
    printf("client init success\n");

    delete_message(&msg);
    nextstep(&msg);
    count_message(&msg);
    nextstep(&msg);
    socket_client_close();
    exit(EXIT_SUCCESS);
}

void insert_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->stuff.book.borrowed = 0;
    msg->stuff.book.on_reading = 0;
    msg->request = req_insert_book_e;
    msg->stuff.book.code[0] = 3;
    msg->stuff.book.code[1] = 0x31;
    strcpy(msg->stuff.book.name, "撒哈啦的故事");
    strcpy(msg->stuff.book.author, "三毛");
    strcpy(msg->stuff.book.label, "流浪文学, 自由生活");
    strcpy(msg->stuff.book.encoding_time, "20190204下午");
}

void delete_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->request = req_delete_book_e;
    msg->stuff.book.code[2] = 8;
}

void update_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->request = req_update_book_e;
    msg->stuff.book.code[0] = 3;
    msg->stuff.book.code[1] = 0x21;
    msg->stuff.book.code[2] = 25;
    strcpy(msg->stuff.book.name, "三毛流浪记");
    strcpy(msg->stuff.book.author, "张乐平");
    strcpy(msg->stuff.book.label, "国产漫画,穷苦生后,儿童,希望");
    msg->stuff.book.borrowed = 1;
    strcpy(msg->stuff.book.encoding_time, "20190203早");
}

void find_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->request = req_find_book_e;
    msg->stuff.book.code[0] = NON_SENSE_INT;
    msg->stuff.book.code[1] = NON_SENSE_INT;
    msg->stuff.book.code[2] = NON_SENSE_INT;
    strcpy(msg->stuff.book.name, "-");
    strcpy(msg->stuff.book.author, "-");
    strcpy(msg->stuff.book.label, "-");
    msg->stuff.book.borrowed = 0;
    msg->stuff.book.on_reading = 0;
}

void nextstep(message_cs_t *msg)
{
    int res;
    int tempPos;
    book_count_t bc;

    res = socket_client_send_request(msg);
    if(!res){
        fprintf(stderr, "client send request error.\n");
        exit(EXIT_FAILURE);
    }
    printf("client send request success!\n");

    do{
        res = socket_client_get_response(msg);
        if(!res){
            fprintf(stderr, "client get response error.\n");
            exit(EXIT_FAILURE);
        }
        if(msg->request == req_find_book_e){
            if(msg->response == r_failed)break;
            if(msg->response == r_find_end){
                printf("Client find no more.\n");
                break;
            }else if(msg->response == r_find_end_more){
                tempPos = msg->stuff.book.code[2];
                find_message(msg);
                msg->stuff.book.code[2] = tempPos;
                res = socket_client_send_request(msg);
                if(!res){
                    fprintf(stderr, "client send request error.\n");
                    exit(EXIT_FAILURE);
                }
                printf("new find request send!\n");
            }else{
                printf("%d %s %s %s\n", msg->stuff.book.code[2], msg->stuff.book.name, msg->stuff.book.author, msg->stuff.book.label);
            }
        }else{
            if(msg->request == req_count_book_e){
                bc = *(book_count_t *)&msg->extra_info;
                printf("all: %d, borrowed: %d, on_reading: %d, unsorted: %d\n",\
                        bc.books_all, bc.books_borrowed, bc.books_on_reading, bc.books_unsorted);
            }
            break;
        }
    }while(1);

    printf("client get response ok!\n");
    if(msg->response == r_success)
    {
        printf("client r_success!\n");
    }else{
        printf("client r_failed %s\n", msg->error_text);
    }
}

void count_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->stuff.book.code[0] = NON_SENSE_INT;
    msg->stuff.book.code[1] = NON_SENSE_INT;
    msg->request = req_count_book_e;
}
