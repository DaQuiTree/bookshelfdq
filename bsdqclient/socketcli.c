#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/aes.h>

#include "socketcom.h"
#include "clisrv.h"

#include "aes/aes_options.h"

static int client_sockfd = -1;
static const unsigned short bsdqsrv_port = 51600;

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
        getchar();
        return(0);
    }

    //设置接收超时
    struct timeval tv;
    tv.tv_sec = SOCKET_RECV_TIMEOUT; 
    tv.tv_usec = 0;
    res = setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    if( res < 0 ){
#if DEBUG_TRACE
        perror("setsockopt()");
#endif
        return(0);
    }

    res = connect(client_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (res == -1){
#if DEBUG_TRACE
        perror("connect()");
#endif 
        getchar();
        return(0);
    }

    //初始化AES加密
    res = AES_init();
    if( res == 0 ){
#if DEBUG_TRACE
        fprintf(stderr, "AES_INIT() failed.\n");
#endif
        getchar();
        return(0);
    }

    return(1);
}

void socket_client_close(void)
{
    client_sockfd = -1;
    close(client_sockfd);
}


int socket_client_send_request(message_cs_t *msg)
{
    int nwrite, encrypt_size;
    unsigned char encrypt_stream[BUFSIZ];
    int msg_size = sizeof(message_cs_t);
    
    if(client_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, " socket_client_send_request(): client fd has not been initialize yet.\n");
#endif
        return 0;
    }

    //加密传输
    encrypt_size = encrypt((unsigned char *)msg, msg_size, encrypt_stream);
    if(encrypt_size == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_srv_process_request(): encrypt() failed");
#endif
        return 0;
    }
    nwrite = send(client_sockfd, encrypt_stream, encrypt_size, 0);
    if (nwrite == -1)
    {
#if DEBUG_TRACE
        perror(" socket_client_send_request(): client write()");
#endif
        return 0;
    }
#if DEBUG_TRACE
    fprintf(stderr, " socket_client_send_request(): client write %d bytes.\n", nwrite);
#endif

    return 1;
}

int socket_client_get_response(message_cs_t *msg)
{
    int nread, res;
    int msg_size = sizeof(message_cs_t);
    int encrypt_size;
    unsigned char encrypt_stream[BUFSIZ];

    if(client_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_client_get_response(): client fd has not been initialize yet.\n");
#endif
        return 0;
    }

    if(msg_size % AES_BLOCK_SIZE == 0)
        encrypt_size = msg_size;
    else
        encrypt_size = (msg_size/AES_BLOCK_SIZE + 1)*AES_BLOCK_SIZE;

    nread = recv(client_sockfd, encrypt_stream, encrypt_size, MSG_WAITALL);
    if (nread == -1)
    {
#if DEBUG_TRACE
        perror("socket_client_get_response(): read()");
#endif
        return 0;
    }

    //数据包长度错误
    if (nread != encrypt_size){
#if DEBUG_TRACE
        fprintf(stderr, "socket_client_get_response(): bytes read doesn't match required.");
        printf("nread: %d, encrypt_size:%d\n",nread, encrypt_size );
#endif
        return 0;
    }

    //数据包解密
    res = decrypt(encrypt_stream, (unsigned char *)msg, nread);         
    if(res == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_client_get_response(): decrypt() failed");
#endif
        return 0;
    }

#if DEBUG_TRACE
    fprintf(stderr, "socket_client_get_response(): read %d bytes.\n", nread);
#endif

    return 1;
}

int socket_client_send_heartbeat(heartbeat_cs_t *hb)
{
    int nwrite, encrypt_size;
    unsigned char encrypt_stream[BUFSIZ];
    int hb_size = sizeof(heartbeat_cs_t);
    
    if(client_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, " socket_client_send_request(): client fd has not been initialize yet.\n");
#endif
        return 0;
    }

    //加密传输
    encrypt_size = encrypt((unsigned char *)hb, hb_size, encrypt_stream);
    if(encrypt_size == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_srv_process_request(): encrypt() failed");
#endif
        return 0;
    }
    nwrite = send(client_sockfd, encrypt_stream, encrypt_size, 0);
    if (nwrite == -1)
    {
#if DEBUG_TRACE
        perror(" socket_client_send_request(): client write()");
#endif
        return 0;
    }
#if DEBUG_TRACE
    fprintf(stderr, " socket_client_send_request(): client write %d bytes.\n", nwrite);
#endif

    return 1;
}
