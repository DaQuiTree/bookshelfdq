#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socketcom.h"
#include "clisrv.h"

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
        return(0);
    }

    res = connect(client_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (res == -1){
#if DEBUG_TRACE
        perror("connect()");
#endif 
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
    int nwrite;
    
    if(client_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, " socket_client_send_request(): client fd has not been initialize yet.\n");
#endif
        return 0;
    }
    nwrite = write(client_sockfd, msg, sizeof(message_cs_t));
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
    int nread;

    if(client_sockfd == -1){
#if DEBUG_TRACE
        fprintf(stderr, "socket_client_get_response(): client fd has not been initialize yet.\n");
#endif
        return 0;
    }
    nread = read(client_sockfd, msg, sizeof(message_cs_t));
    if (nread == -1)
    {
#if DEBUG_TRACE
        perror("socket_client_get_response(): read()");
#endif
        return 0;
    }
#if DEBUG_TRACE
    fprintf(stderr, "socket_client_get_response(): read %d bytes.\n", nread);
#endif
    return 1;
}
