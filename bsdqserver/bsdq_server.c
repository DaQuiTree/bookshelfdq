#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "clisrv.h"
#include "socketcom.h"
#include "srvdb.h"

int main()
{
    int server_running = 1;
    int res;
    int client_fd;

    if(!srvdb_init()){
        fprintf(stderr, "server database init error.\n");
        exit(EXIT_FAILURE);
    }

    if(!srvdb_connect("127.0.0.1", 0)){
        fprintf(stderr, "server database connect error.\n");
        exit(EXIT_FAILURE);
    }

    if(!srvdb_user_archive_init("daqui")){
        fprintf(stderr, "user archive init error.\n");
        exit(EXIT_FAILURE);
    }

    if(!socket_srv_init()){
        fprintf(stderr, "server socket init error.\n");
        exit(EXIT_FAILURE);
    }

    printf("server init ok.\n");
    while(server_running)
    {
        res = socket_srv_wait();
        if(!res){
            fprintf(stderr, "server running error while waiting.\n");
            exit(EXIT_FAILURE);
        }

        printf("server wait over.\n");
        while((client_fd = socket_srv_fetch_client()) != SOCKET_FETCH_END)
        {
            if (client_fd == SOCKET_FETCH_ERR){
                fprintf(stderr, "server running error when fetch client fd.\n");
                exit(EXIT_FAILURE);
            }
            printf("server fetch one.\n");
            socket_srv_process_request(client_fd);
            printf("server process a request.\n");
        };
    }

    socket_srv_close(); //不应该来到这里
    exit(EXIT_FAILURE);
}