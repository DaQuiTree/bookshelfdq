#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clisrv.h"
#include "socketcom.h"

void init_message(message_cs_t *msg);

int main(void)
{
    message_cs_t msg;
    int res;

    init_message(&msg);

    if(!socket_client_init("127.0.0.1"))
    {
        fprintf(stderr, "client init error.\n");
        exit(EXIT_FAILURE);
    }
    printf("client init success\n");

    res = socket_client_send_request(&msg);
    if(!res){
        fprintf(stderr, "client send request error.\n");
        exit(EXIT_FAILURE);
    }
    printf("client send request success!\n");

    res = socket_client_get_response(&msg);
    if(!res){
        fprintf(stderr, "client get response error.\n");
        exit(EXIT_FAILURE);
    }
    printf("client get response ok!\n");
    if(msg.response == r_success)
    {
        printf("client request effected!\n");
    }else{
        printf("client not get what it wants: %s\n", msg.error_text);
    }

    socket_client_close();
    exit(EXIT_SUCCESS);
}

void init_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->request = req_insert_book_e;
    msg->stuff.book.code[0] = 4;
    msg->stuff.book.code[1] = 0x51;
    strcpy(msg->stuff.book.name, "史记");
    strcpy(msg->stuff.book.author, "司马迁");
    strcpy(msg->stuff.book.label, "史家经典, 古籍, 政治历史");
    strcpy(msg->stuff.book.encoding_time, "20190202晚");
}
