#ifndef _SRVDB_H_
#define _SRVDB_H_

#include "clisrv.h"

int srvdb_init(void);
int srvdb_connect(const char* hostname, unsigned int port);
int srvdb_user_archive_init(const char* username);

int srvdb_book_insert(message_cs_t *msg);
int srvdb_book_delte(message_cs_t *msg);
int srvdb_book_update(message_cs_t *msg);
int srvdb_book_find(message_cs_t *msg, int *num_rows);
int srvdb_book_fetch_result(message_cs_t *msg);

int srvdb_shelf_insert(message_cs_t *msg);
int srvdb_shelf_delte(message_cs_t *msg);
int srvdb_shelf_update(message_cs_t *msg);
int srvdb_shelf_find(message_cs_t *msg, int *num_rows);
int srvdb_shelf_fetch_result(message_cs_t *msg);

//检索相关的通用函数
void srvdb_free_result(void);

#endif
