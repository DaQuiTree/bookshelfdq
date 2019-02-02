#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "clisrv.h"
#include "srvdb.h"

static int server_sockfd = -1;
static int client_sockfd = -1;

static fd_set readfds, recordfds;
static int max_fd_num = 0;

static const unsigned short bsdqsrv_port = 51600;

static void find_max_fd(int *max_fd);

//server
int socket_srv_init(void)
{
    struct sockaddr_in server_addr;
    int result;

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( server_sockfd == -1 ){
#if DEBUG_TRACE
        perror("socket()");
#endif
        return(0);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(bsdq_port);

    result = bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if( result == -1 ){
#if DEBUG_TRACE
        perror("bind()");
#endif
        return(0);
    }

    result = listen(server_sockfd, 5);
    if( result == -1 ){
#if DEBUG_TRACE
        perror("listen()");
#endif
        return(0);
    }

    FD_ZERO(&readfds);
    FD_ZERO(&recordfds);
    return(1);
}

void socket_srv_close(void)
{
    FD_ZERO(&readfds);
    FD_ZERO(&recordfds);
    max_fd_num = 0;
    server_sockfd = -1;
    close(server_sockfd);
}

int socket_srv_wait(void)
{
    int res = 0;

    if(server_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "server fd has not been initialize yet.\n");
#endif
        return 0;
    }
    readfds = recordfds;
    res = select(max_fd_num+1, &readfds, NULL, NULL, (struct timeval *)0);
    if(res < 1){
#if DEBUG_TRACE
        perror("select()");
#endif
        return(0);
    }

    return(1);
}

int socket_srv_fetch_client(void)
{
    static int fd_loop = 0;
    struct sockaddr_in client_addr;
    int newfd, nread, addr_len;

    if(server_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "server fd has not been initialize yet.\n");
#endif
        return 0;
    }

    //描述符集中有成员?
    while (fd_loop <= max_fd_num){
        if (FD_ISSET(fd_loop, &readfds)){
            if (fd_loop == server_sockfd){//client连接请求
                newfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);
                if (newfd == -1){
#if DEBUG_TRACE
                    perror("accept()");
#endif
                    return(SOCKET_FETCH_ERR);
                }
                FD_SET(newfd, &recordfds);
                newfd > max_fd_num ? max_fd_num = newfd:;
            }else{//数据ready
                ioctl(fd_loop, FIONREAD, &nread);
                if(nread == 0){
                    close(fd_loop);
                    FD_CLR(fd_loop, &recordfds);
                    find_max_fd(&max_fd_num);
                }else{
                    fd_loop++;
                    return (fd_loop-1);
                }
            }
        }
        fd_loop++;
    }
    fd_loop = 0;

#if DEBUG_TRACE
    fprintf(stdout, "loop of server fetch client fd is over.\n");
#endif
    return (SOCKET_FETCH_END);
}

int socket_srv_process_request(int client_fd)
{
    int nread, nwrite, nrows;
    message_cs_t msg;
    int msg_size = sizeof(msg);

    if(server_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "server fd has not been initialize yet.\n");
#endif
        return 0;
    }
    
    nread = read(client_fd, &msg, msg_size);
    //待接收的数据长度错误
    if (nread != msg_size){
        if (nread < 0){
#if DEBUG_TRACE
            perror("read()");
#endif
            return 0;
        }else{
#if DEBUG_TRACE
            fprintf(stderr, "read(msg) error: Server recieved incorrect num of message.\n");
#endif
            msg.response = r_failed;
            strcpy(msg.error_text, "Server recieved incorrect num of message.");
            nwrite = write(client_fd, &msg, msg_size);
            if (nwrite < 0){
#if DEBUG_TRACE
                perror("write()");
#endif
                return(0);
            }
#if DEBUG_TRACE
            fprintf(stderr, "read(msg) error: server write %d bytes to client\n", nwrite);
#endif
            return(1);
        }
    }

    msg.response = r_success;
    msg.error_text[0] = '\0';
    //处理正常数据 
    switch(msg.request)
    {
        case req_insert_book_e:
            if(!srvdb_book_insert(&msg))msg.response = r_failed;
            break;
        case req_delete_book_e:
            if(!srvdb_book_delete(&msg))msg.response = r_failed;
            break;
        case req_update_book_e:
            if(!srvdb_book_update(&msg))msg.response = r_failed;
            break;
        case req_find_book_e:
            if(!srvdb_book_find(&msg, &nrows)){
                msg.response = r_failed;
                srvdb_free_result();
                break;
            }
            while(srvdb_book_fetch_result(&msg)){
                nwrite = write(client_fd, &msg, msg_size);
                if (nwrite < 0){
#if DEBUG_TRACE
                    perror("write()");
#endif
                    return(0);
                }
#if DEBUG_TRACE
                fprintf(stderr, "write: server write %d bytes to client\n", nwrite);
#endif
            }
            return(1);
        case req_build_shelf_e:
            if(!srvdb_shelf_build(&msg))msg.response = r_failed;
            break;
        case req_remove_shelf_e:
            if(!srvdb_shelf_remove(&msg))msg.response = r_failed;
            break;
        case req_find_shelf_e:
            if(!srvdb_shelf_find(&msg, &nrows)){
                msg.response = r_failed;
                break;
            }
            while(srvdb_shelf_fetch_result(&msg)){
                nwrite = write(client_fd, &msg, msg_size);
                if (nwrite < 0){
#if DEBUG_TRACE
                    perror("write()");
#endif
                    return(0);
                }
#if DEBUG_TRACE
                fprintf(stderr, "write: server write %d bytes to client\n", nwrite);
#endif
            }
            return(1);
        default:
            msg.response = r_failed;
            strcpy(msg.error_text, "Undefined request type.");
            break;
    }

    nwrite = write(client_fd, &msg, msg_size);
    if (nwrite < 0){
#if DEBUG_TRACE
        perror("write()");
#endif
        return(0);
    }
#if DEBUG_TRACE
    fprintf(stderr, "write: server write %d bytes to client\n", nwrite);
#endif
}


//client
int socket_client_init(char *host)
{
    int res;
    struct sockaddr_in server_addr;


    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(host);
    server_addr.sin_port = htons(bsdqsrv_port);

    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd == -1)
    {
#if DEBUG_TRACE
        perror("socket()");
#endif 
        return(0);
    }

    res = connect(client_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (res == -1){
#if DEBUG_TRACE
        perror("connect()");
#endif 
        return(0);
    }
}

void socket_client_close(void)
{
    client_sockfd = -1;
    close(client_sockfd);
}


int socket_client_send_request(message_cs_t *msg)
{
    int nwrite;
    
    if(client_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "client fd has not been initialize yet.\n");
#endif
        return 0;
    }
    nwrite = write(client_sockfd, msg, sizeof(message_cs_t));
    if (nwrite == -1)
    {
#if DEBUG_TRACE
        perror("client write()");
#endif
        return 0;
    }
#if DEBUG_TRACE
    fprintf(stderr, "Client write %d bytes.\n", nwrite);
#endif
    return 1;
}

int socket_client_get_response(message_cs_t *msg)
{
    int nread;

    if(client_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "client fd has not been initialize yet.\n");
#endif
        return 0;
    }
    nread = read(client_sockfd, msg, sizeof(message_cs_t));
    if (nread == -1)
    {
#if DEBUG_TRACE
        perror("client read()");
#endif
        return 0;
    }
#if DEBUG_TRACE
    fprintf(stderr, "Client read %d bytes.\n", nread);
#endif
    return 1;
}

static void find_max_fd(int *max_fd)
{
    int i;

    for(i = *max_fd; i >= 0; i--)
    {
        if (FD_ISSET(i, &recordfds)){
            *max_fd = i;
            return;
        }
    }

}
