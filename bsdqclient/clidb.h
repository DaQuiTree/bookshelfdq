#ifndef _CLIDB_H_
#define _CLIDB_H_

#include "clisrv.h"

int clidb_init(char *user);

//书架相关
int clidb_shelf_insert(shelf_entry_t *user_shelf);
int clidb_shelf_synchronize(shelf_entry_t *user_shelf);
int clidb_shelf_not_syncs(int shelfno);
int clidb_shelf_delete(int shelfno);
int clidb_shelf_get(int shelfno, shelf_entry_t *user_shelf);
int clidb_shelf_exists(int shelfno);
int shelf_record_sort(void);

//书籍相关
void clidb_book_reset(void);//清空书籍相关的dbm
int clidb_book_insert(book_entry_t *user_book);
void clidb_book_search_step(int step);//设置查找的数量
int clidb_book_backward_mode(void);//设置倒序查找模式
int clidb_book_get(book_entry_t *user_book);

#endif
