#ifndef _SOCKETCOM_H_
#define _SOCKETCOM_H_

#include "clisrv.h"

#define SOCKET_FETCH_ERR (-1)
#define SOCKET_FETCH_END (-2)

//server
int socket_srv_init(void);
int socket_srv_wait(void);
int socket_srv_fetch_client(void);
int socket_srv_process_request(int client_fd);
void socket_srv_close(void);

//client
int socket_client_init(char* host);
int socket_client_send_request(message_cs_t *msg);
int socket_client_get_response(message_cs_t *msg);
void socket_client_close(void);

#endif

