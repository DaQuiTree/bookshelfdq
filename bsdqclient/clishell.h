#ifndef _CLISHELL_H_
#define _CLISHELL_H_

int client_initialize(char* host, char* user);
int client_shelves_info_sync(void);
int client_books_info_sync(void); 

#endif
