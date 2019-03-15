#ifndef _SRVSHELL_H_
#define _SRVSHELL_H_

#define RESET_FLAG (0x80)

void server_persist_signals(void);
int get_option(int argc, char *argv[], unsigned char *flag);
void get_mysql_password(char *password);
int reset_admin_password(void);
void help_manul(void);

#endif

