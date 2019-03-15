#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "clisrv.h"
#include "socketcom.h"
#include "srvdb.h"
#include "srvshell.h"

int main(int argc, char* argv[])
{
    char my_password[MYSQL_PW_LEN+1] = {0};
    int server_running = 1;
    int res;
    int client_fd;
    unsigned char optflag = 0;

    if(!get_option(argc, argv, &optflag))
        exit(EXIT_SUCCESS);

    fprintf(stdout, "Verify bsdq@mysql password: ");
    get_mysql_password(my_password);

    if(!srvdb_init()){
        fprintf(stderr, "server database init error.\n");
        exit(EXIT_FAILURE);
    }

    if(!srvdb_connect("127.0.0.1", my_password, 0)){
        fprintf(stderr, "server database connect error.\n");
        exit(EXIT_FAILURE);
    }

    if(!srvdb_accounts_table_init()){
        fprintf(stderr, "server init accounts table error.\n");
        srvdb_close();
        exit(EXIT_FAILURE);
    }

    //屏蔽信号安全退出
    server_persist_signals();

    //需要重置管理员密码
    if(optflag & (RESET_FLAG)){
        (void)reset_admin_password();
        srvdb_close();
        exit(EXIT_SUCCESS);
    }

    if(!socket_srv_init()){
        fprintf(stderr, "server socket init error.\n");
        srvdb_close();
        exit(EXIT_FAILURE);
    }

    printf("server init ok.\n");
    while(server_running)
    {
        res = socket_srv_wait();
        if(!res){
            fprintf(stderr, "server running error while waiting.\n");
            srvdb_close();
            exit(EXIT_FAILURE);
        }

        printf("server wait over.\n");
        while((client_fd = socket_srv_fetch_client()) != SOCKET_FETCH_END)
        {
            if (client_fd == SOCKET_FETCH_ERR){
                fprintf(stderr, "server running error when fetch client fd.\n");
                srvdb_close();
                exit(EXIT_FAILURE);
            }
            printf("server fetch one.\n");
            socket_srv_process_request(client_fd);
            printf("server process a request.\n");
        };
    }

    srvdb_close();
    socket_srv_close(); //不应该来到这里
    exit(EXIT_FAILURE);
}

