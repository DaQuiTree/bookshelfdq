#ifndef _CLISHELL_H_
#define _CLISHELL_H_

#include "clisrv.h"

//通用
int client_start_login(char *host, account_entry_t *user_account, char *errInfo);
int client_start_register(char *host, account_entry_t *user_account, char *errInfo);
int client_shelves_info_sync(shelf_count_t *sc);
int client_shelves_count_sync(shelf_count_t *sc); 
int client_books_count_sync(book_count_t *bc); 

//搜索图书请求
int client_searching_book(int bookno, book_entry_t *search_entry);

//打造书架请求
int client_build_shelf(shelf_entry_t *user_shelf, char *errInfo);

//浏览书架相关请求
int client_shelf_insert_book(book_entry_t *user_book, char *errInfo);
int client_shelf_count_book(int shelfno, book_count_t *user_bc);
int client_shelf_loading_book(int shelfno, int bookno);
int client_shelf_delete_book(book_entry_t *user_book, int dbm_pos);
int client_shelf_collecting_book(int shelfno, book_entry_t *user_book, int dbm_pos);
int client_shelf_tagging_book(int shelfno, book_entry_t *user_book, int dbm_pos);
int client_shelf_moving_book(int shelfno, book_entry_t *user_book, int dbm_pos, int move_action);
int client_shelf_abandon_books(int shelfno);
int client_shelf_unsorted_books(int shelfno);
int client_shelf_delete_itself(int shelfno);

//账户操作
int client_register_account(account_entry_t *user_account, char *errInfo);
int client_verify_account(account_entry_t *user_account, int init_mode, char *errInfo);

#endif
