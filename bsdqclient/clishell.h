#ifndef _CLISHELL_H_
#define _CLISHELL_H_

#include "clisrv.h"

int client_initialize(char* host, char* user);
int client_shelves_info_sync(shelf_count_t *sc);
int client_books_info_sync(book_count_t *bc); 

int client_shelf_insert_book(book_entry_t *user_book);
int client_shelf_count_book(int shelfno, book_count_t *user_bc);
int client_shelf_loading_book(int shelfno, int bookno);
int client_shelf_delete_book(int shelfno, book_entry_t *user_book, int dbm_pos);
int client_shelf_tagging_book(int shelfno, book_entry_t *user_book, int dbm_pos);
int client_shelf_moving_book(int shelfno, book_entry_t *user_book, int dbm_pos, int move_action);
int client_shelf_abandon_books(int shelfno);
int client_shelf_unsorted_books(int shelfno);
int client_shelf_delete_itself(int shelfno);
#endif
