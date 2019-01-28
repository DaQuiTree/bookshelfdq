#ifndef _SRVDB_H_
#define _SRVDB_H_

#include "clisrv.h"


int srvdb_init(void);
int srvdb_connect(const char* hostname, unsigned int port);
int srvdb_user_archive_init(const char* username);

int srvdb_book_insert(message_cs_t *msg);
int srvdb_book_get(message_cs_t *msg);
int srvdb_book_delte(message_cs_t *msg);
int srvdb_book_update(message_cs_t *msg);
int srvdb_book_find(message_cs_t *msg);

int srvdb_shelf_insert(message_cs_t *msg);
int srvdb_shelf_get(message_cs_t *msg);
int srvdb_shelf_delte(message_cs_t *msg);
int srvdb_shelf_update(message_cs_t *msg);
int srvdb_shelf_find(message_cs_t *msg);

#endif
