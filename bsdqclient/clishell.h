#ifndef _CLISHELL_H_
#define _CLISHELL_H_

#include "clisrv.h"

int client_initialize(char* host, char* user);
int client_shelves_info_sync(shelf_count_t *sc);
int client_books_info_sync(book_count_t *bc); 

int client_shelf_loading_book(int shelfno, int bookno);
int client_shelf_delete_book(int shelfno, int bookno);
int client_shelf_tagging_book(int shelfno, book_entry_t *bookno);
#endif
