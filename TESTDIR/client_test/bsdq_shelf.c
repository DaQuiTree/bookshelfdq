#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clisrv.h"
#include "socketcom.h"

void insert_message(message_cs_t *msg);
void delete_message(message_cs_t *msg);
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

    count_message(&msg);
    nextstep(&msg);
    socket_client_close();
    exit(EXIT_SUCCESS);
}

void insert_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->request = req_build_shelf_e;
    strcpy(msg->stuff.shelf.name, "5141小阳台书架");
    msg->stuff.shelf.nfloors = 2;
    msg->stuff.shelf.ndepth[0] = 1;
    msg->stuff.shelf.ndepth[1] = 1;
    strcpy(msg->stuff.shelf.building_time, "20190204下午");
}

void delete_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->request = req_remove_shelf_e;
    msg->stuff.shelf.code = 13;
}

/*void update_message(message_cs_t *msg)*/
/*{*/
    /*strcpy(msg->user, "daqui");*/
    /*msg->request = req_update_shelf_e;*/
    /*msg->stuff.shelf.code = 12;*/
    /*strcpy(msg->stuff.shelf.name, "徐鹏书架");*/
    /*msg->stuff.shelf.nfloors = 4;*/
    /*msg->stuff.shelf.ndepth[0] = 2;*/
    /*msg->stuff.shelf.ndepth[1] = 2;*/
    /*msg->stuff.shelf.ndepth[2] = 1;*/
    /*msg->stuff.shelf.ndepth[3] = 1;*/
    /*strcpy(msg->stuff.shelf.building_time, "20190204下午");*/
/*}*/

void find_message(message_cs_t *msg)
{
    strcpy(msg->user, "daqui");
    msg->request = req_find_shelf_e;
    msg->stuff.shelf.code = NON_SENSE_INT;
}

void nextstep(message_cs_t *msg)
{
    int res;
    int tempPos, i;
    shelf_count_t sc;

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
        if(msg->request == req_find_shelf_e){
            if(msg->response == r_failed)break;
            if(msg->response == r_find_end){
                printf("Client find no more.\n");
                break;
            }else if(msg->response == r_find_end_more){
                tempPos = msg->stuff.shelf.code;
                find_message(msg);
                msg->stuff.shelf.code = tempPos;
                res = socket_client_send_request(msg);
                if(!res){
                    fprintf(stderr, "client send request error.\n");
                    exit(EXIT_FAILURE);
                }
                printf("new find request send!\n");
            }else{
                printf("%d %s %d %s | ", msg->stuff.shelf.code, msg->stuff.shelf.name, msg->stuff.shelf.nfloors, msg->stuff.shelf.building_time);
                for(i = 0; i < msg->stuff.shelf.nfloors; i++){
                    printf("%d ", msg->stuff.shelf.ndepth[i]);
                }
                printf("\n");
            }
        }else{
            if(msg->request == req_count_shelf_e){
                sc = *(shelf_count_t *)&msg->extra_info;
                printf("all: %d\n", sc.shelves_all);
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
    msg->request = req_count_shelf_e;
}
