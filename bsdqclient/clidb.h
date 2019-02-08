#ifndef _CLIDB_H_
#define _CLIDB_H_

#include "clisrv.h"

int clidb_init(char *user);

int clidb_shelf_insert(shelf_entry_t *user_shelf);
int clidb_shelf_synchronize(shelf_entry_t *user_shelf);
int clidb_shelf_delete(int shelfno);
int clidb_shelf_get(shelf_entry_t *user_shelf);
int clidb_shelf_exits(int shelfno);

int clidb_book_reset(void);
int clidb_book_insert(book_entry_t *user_book);
int clidb_book_backward_get(shelf_entry_t *user_book);
int clidb_book_forward_get(shelf_entry_t *user_book);
#endif
