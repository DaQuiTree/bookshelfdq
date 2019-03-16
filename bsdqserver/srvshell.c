#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include "socketcom.h"
#include "clisrv.h"
#include "srvdb.h"
#include "srvshell.h"

static void safty_exit(int sig)
{
    srvdb_close();
    socket_srv_close();
    exit(EXIT_FAILURE);
}

int get_option(int argc, char *argv[], unsigned char *flag, char optarg_copy[][MAX_ARGV_LEN+1])
{
    int opt;

    struct option longopts[] = {
        {"admin-reset", 0, NULL, 'a'},
        {"help", 0, NULL, 'h'},
        {0,0,0,0}
    };

    while((opt = getopt_long(argc, argv, ":hap:", longopts, NULL)) != -1){
        switch(opt){
            case 'h':
                help_manul();
                return(0);
            case 'a':
                *flag |= RESET_FLAG;
                break;
            case 'p':
                *flag |= PASSWORD_FLAG;
                strncpy(optarg_copy[0], optarg, MAX_ARGV_LEN);
                break;
            case '?':
                help_manul();
                return(0);
        }
    }

    return(1);
}

void help_manul(void)
{
    fprintf(stdout, "\nUsage: ./bsdq_server [option] \n\n");
    fprintf(stdout, "Options: \n");
    fprintf(stdout, "       -p              directly input password.\n");
    fprintf(stdout, "       --admin-reset   reset administrator password.\n\n");
}

void get_mysql_password(char *password)
{
    struct termios oldsettings, newsettings;

    //关闭回显
    tcgetattr(fileno(stdin), &oldsettings);
    newsettings = oldsettings;
    newsettings.c_lflag &= ~ECHO;
    
    if(tcsetattr(fileno(stdin), TCSAFLUSH, &newsettings) != 0)return;
    fgets(password, MYSQL_PW_LEN, stdin);
    printf("\n");

    //回显
    tcsetattr(fileno(stdin), TCSANOW, &oldsettings);
    password[strlen(password)-1] = '\0';

}

int reset_admin_password(void)
{   
    message_cs_t msg;

    char password[MYSQL_PW_LEN+1] = {0};
    char password_auth[MYSQL_PW_LEN+1] = {0};
    
    fprintf(stdout, "Verify admin password: ");
    get_mysql_password(password);
    strcpy(msg.user, ADMIN_DEFAULT_ACCOUNT);
    msg.request = req_verify_account_e;
    msg.stuff.account.type = account_admin_e;
    strcpy(msg.stuff.account.name, ADMIN_DEFAULT_ACCOUNT);
    strcpy(msg.stuff.account.password, password);

    if(!srvdb_account_verify(&msg)){
        fprintf(stdout, "Authenticate Failed.\n");
        return(0);
    }

    fprintf(stdout, "new password: ");
    get_mysql_password(password);
    fprintf(stdout, "again: ");
    get_mysql_password(password_auth);
    if(!strcmp(password, password_auth)){
        strcpy(msg.stuff.account.password, password_auth);
        if(!srvdb_account_update(&msg)){
            fprintf(stdout, "Error: change password failed.\n");
        }else{
            fprintf(stdout, "Well Done.\n");
            return(1);
        }
    }else{
        fprintf(stdout, "Error: different input.\n");
    }

    return(0);
}

void server_persist_signals(void)
{
    (void)signal(SIGINT, safty_exit);
}
