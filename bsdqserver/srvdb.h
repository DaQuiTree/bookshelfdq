#ifndef _SRVDB_H_
#define _SRVDB_H_

#include "clisrv.h"

int srvdb_init(void);
void srvdb_close(void);
int srvdb_connect(const char* hostname, const char* password, unsigned int port);
int srvdb_accounts_table_init(void);
int srvdb_user_archive_init(const char* username);

int srvdb_book_insert(message_cs_t *msg);
int srvdb_book_delete(message_cs_t *msg);
int srvdb_book_update(message_cs_t *msg);
int srvdb_book_find(message_cs_t *msg, int *num_rows);
int srvdb_book_fetch_result(message_cs_t *msg);
int srvdb_book_count(message_cs_t *msg);

int srvdb_shelf_build(message_cs_t *msg);
int srvdb_shelf_remove(message_cs_t *msg);
int srvdb_shelf_update(message_cs_t *msg);
int srvdb_shelf_find(message_cs_t *msg, int *num_rows);
int srvdb_shelf_fetch_result(message_cs_t *msg);
int srvdb_shelf_count(message_cs_t *msg);

int srvdb_account_verify(message_cs_t *msg);
int srvdb_account_register(message_cs_t *msg);
int srvdb_account_update(message_cs_t *msg);

//检索相关的通用函数
void srvdb_free_result(void);

#endif
