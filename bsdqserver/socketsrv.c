#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/aes.h>

#include "socketcom.h"
#include "clisrv.h"
#include "srvdb.h"
#include "aes/aes_options.h"

static int server_sockfd = -1;

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
    server_addr.sin_port = htons(bsdqsrv_port);

    //设置套接字选项避免地址使用错误
    int on = 1;
    result = setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if( result < 0 ){
#if DEBUG_TRACE
        perror("setsockopt()");
#endif
        return(0);
    }

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

    //初始化AES加密
    result = AES_init();
    if( result == 0 ){
#if DEBUG_TRACE
        fprintf(stderr, "AES_INIT() failed.\n");
#endif
        return(0);
    }

    FD_ZERO(&readfds);
    FD_ZERO(&recordfds);
    FD_SET(server_sockfd, &recordfds);
    max_fd_num = server_sockfd;

    return(1);
}

void socket_srv_close(void)
{
    FD_ZERO(&readfds);
    FD_ZERO(&recordfds);
    max_fd_num = 0;
    if(server_sockfd != -1)
        close(server_sockfd);
    server_sockfd = -1;
}

int socket_srv_wait(void)
{
    int res = 0;

    if(server_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_srv_wait(): server fd has not been initialize yet.\n");
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
    unsigned int addr_len = 0;
    int newfd, nread;

    if(server_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_srv_fetch_client(): server fd has not been initialize yet.\n");
#endif
        return SOCKET_FETCH_ERR;
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
                newfd > max_fd_num ? max_fd_num = newfd : 0;
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
    fprintf(stdout, "socket_srv_fetch_client(): loop of server fetch client fd is over.\n");
#endif
    return (SOCKET_FETCH_END);
}

int socket_srv_process_request(int client_fd)
{
    int nread, nwrite, nrows, res;
    message_cs_t msg;
    
    unsigned char encrypt_stream[BUFSIZ];
    int msg_size = sizeof(msg);
    int hb_size = sizeof(heartbeat_cs_t);
    int encrypt_size, encrypt_size_hb;

    if(server_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_srv_process_request(): server fd has not been initialize yet.\n");
#endif
        return 0;
    }

    nread = recv(client_fd, encrypt_stream, BUFSIZ, 0);

    //计算加密后的心跳包长度
    if(msg_size % AES_BLOCK_SIZE == 0)
        encrypt_size_hb = hb_size;
    else
        encrypt_size_hb = (hb_size/AES_BLOCK_SIZE + 1)*AES_BLOCK_SIZE;

    //如果是心跳包长度,不做处理
    if(nread == encrypt_size_hb){
#if DEBUG_TRACE
        fprintf(stderr, "recieve heartbeat packet from clientfd: %d.\n", client_fd);
#endif
        return 1;
    }

    //计算加密后的数据长度
    if(msg_size % AES_BLOCK_SIZE == 0)
        encrypt_size = msg_size;
    else
        encrypt_size = (msg_size/AES_BLOCK_SIZE + 1)*AES_BLOCK_SIZE;

    //接收的数据长度错误
    if (nread != encrypt_size){
        if (nread < 0){
#if DEBUG_TRACE
            perror("socket_srv_process_request(): read()");
#endif
            return 0;
        }else{
#if DEBUG_TRACE
            fprintf(stderr, "socket_srv_process_request() read error: Server recieved incorrect num(%d) of message.\n", nread);
#endif
            msg.response = r_failed;
            strcpy(msg.error_text, "Server recieved incorrect num(%d) of message.");

            //加密传输
            encrypt_size = encrypt((unsigned char *)&msg, msg_size, encrypt_stream);
            if(encrypt_size == -1){
#if DEBUG_TRACE
                fprintf(stderr, "socket_srv_process_request(): encrypt() failed");
#endif
                return 0;
            }
            nwrite = send(client_fd, encrypt_stream, encrypt_size, 0);
            if (nwrite < 0){
#if DEBUG_TRACE
                perror("socket_srv_process_request(): write()");
#endif
                return(0);
            }
#if DEBUG_TRACE
            fprintf(stderr, "socket_srv_process_request() read error: server write %d bytes to client\n", nwrite);
#endif
            return(1);
        }
    }

    //数据包解密
    res = decrypt(encrypt_stream, (unsigned char *)&msg, nread);         
    if(res == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_srv_process_request(): decrypt() failed");
#endif
        return 0;
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
                break;
            } 
            msg.extra_info[0] = nrows;
            while((res = srvdb_book_fetch_result(&msg)) != FETCH_RESULT_ERR){
                if(res == FETCH_RESULT_END){
                    msg.response = r_find_end; //查询结束
                }else if(res == FETCH_RESULT_END_MORE){
                    msg.response = r_find_end_more; //本次结束需再次查询
                }else{
                    msg.response = r_success;
                }
                //加密传输
                encrypt_size = encrypt((unsigned char *)&msg, msg_size, encrypt_stream);
                if(encrypt_size == -1){
#if DEBUG_TRACE
                    fprintf(stderr, "socket_srv_process_request(): encrypt() failed");
#endif
                    return 0;
                }
                nwrite = send(client_fd, encrypt_stream, encrypt_size, 0);
                if (nwrite < 0){
#if DEBUG_TRACE
                    perror("socket_srv_process_request: write()");
#endif
                    return(0);
                }
#if DEBUG_TRACE
                fprintf(stderr, "socket_srv_process_request(): server write %d bytes to client\n", nwrite);
#endif
                if(res == FETCH_RESULT_END ||  res == FETCH_RESULT_END_MORE)return(1);
            }
            msg.response = r_failed;
            strcpy(msg.error_text, "Find book request failed, try again later.");
            return(0);
        case req_count_book_e:
            if(!srvdb_book_count(&msg))msg.response = r_failed;
            break;
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
            msg.extra_info[0] = nrows;
            while((res = srvdb_shelf_fetch_result(&msg)) != FETCH_RESULT_ERR){
                if(res == FETCH_RESULT_END){
                    msg.response = r_find_end; //查询结束
                }else if(res == FETCH_RESULT_END_MORE){
                    msg.response = r_find_end_more; //本次结束需再次查询
                }else{
                    msg.response = r_success;
                }
                //加密传输
                encrypt_size = encrypt((unsigned char *)&msg, msg_size, encrypt_stream);
                if(encrypt_size == -1){
#if DEBUG_TRACE
                    fprintf(stderr, "socket_srv_process_request(): encrypt() failed");
#endif
                    return 0;
                }
                nwrite = send(client_fd, encrypt_stream, encrypt_size, 0);
                if (nwrite < 0){
#if DEBUG_TRACE
                    perror("socket_srv_process_request(): write() ");
#endif
                    return(0);
                }
#if DEBUG_TRACE
                fprintf(stderr, "socket_srv_process_request(): server write %d bytes to client\n", nwrite);
#endif
                if(res == FETCH_RESULT_END ||  res == FETCH_RESULT_END_MORE)return(1);
            }

            msg.response = r_failed;
            strcpy(msg.error_text, "Find shelf request failed, try again later.");
            return(0);
        case req_count_shelf_e:
            if(!srvdb_shelf_count(&msg))msg.response = r_failed;
            break;
        case req_verify_account_e:
            if(!srvdb_account_verify(&msg)){
                msg.response = r_failed;
            }else{
                //验证通过,进行数据库初始化
                if(msg.extra_info[0] == 1){
                    if(!srvdb_user_archive_init(msg.stuff.account.name))
                        msg.response = r_failed;
                }
            }
            break;
        case req_register_account_e:
            if(!srvdb_account_register(&msg))msg.response = r_failed;
            break;
        default:
            msg.response = r_failed;
            strcpy(msg.error_text, "Undefined request type.");
            return 0;
    }

    //加密传输
    encrypt_size = encrypt((unsigned char *)&msg, msg_size, encrypt_stream);
    if(encrypt_size == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_srv_process_request(): encrypt() failed");
#endif
        return 0;
    }
    nwrite = send(client_fd, encrypt_stream, encrypt_size, 0);
    if (nwrite < 0){
#if DEBUG_TRACE
        perror("socket_srv_process_request(): write()");
#endif
        return(0);
    }
#if DEBUG_TRACE
    fprintf(stderr, "socket_srv_process_request(): server write %d bytes to client\n", nwrite);
#endif
    return(1);
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
