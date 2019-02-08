#ifndef _CLIDB_H_
#define _CLIDB_H_

#include "clisrv.h"

int clidb_init(char *user);

int clidb_shelf_insert(shelf_entry_t *user_shelf);
int clidb_shelf_synchronize(shelf_entry_t *user_shelf);
int clidb_shelf_delete(int shelfno);
int clidb_shelf_get(int shelfno, shelf_entry_t *user_shelf);
int clidb_shelf_exists(int shelfno);

void clidb_book_reset(void);//清空书籍相关的dbm
void clidb_book_set_step(int step);//设置backward或forward的长度
int clidb_book_insert(book_entry_t *user_book);
int clidb_book_backward_get(book_entry_t *user_book);
int clidb_book_forward_get(book_entry_t *user_book);
#endif
