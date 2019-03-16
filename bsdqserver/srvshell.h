#ifndef _SRVSHELL_H_
#define _SRVSHELL_H_

#include "clisrv.h"

#define RESET_FLAG (0x80)
#define PASSWORD_FLAG (0x40)

void server_persist_signals(void);
int get_option(int argc, char *argv[], unsigned char *flag, char optarg_copy[][MAX_ARGV_LEN+1]);
void get_mysql_password(char *password);
int reset_admin_password(void);
void help_manul(void);

#endif

