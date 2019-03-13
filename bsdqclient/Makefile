all: bookMan #test_clidb bsdq_server bsdq_client srvdbmain

CC = gcc
CFLAGS = -Wall -pedantic

#Debug Flags
#DFLAGS = -g -DDEBUG_TRACE=1

#MYSQL
MYSQL_INC_PATH = /usr/include/mysql
MYSQL_LIB_PATH = /usr/lib/mysql
MYSQL_LIB_FILE = -lmysqlclient

#DBM
DBM_INC_PATH = /usr/include/gdbm
DBM_LIB_FILE = -lgdbm_compat -lgdbm

#NCURSES
NC_INC_PATH = /usr/include/ncursesw
NC_LIB_FILE = -lncursesw

#password strength
#CRACKLIB
CRACKLIB = /usr/local/share/cracklib/pw_dict
CRACKLIB_LIB_FILE = -lcrack
CRFLAGS = -DHAVE_CRACKLIB -DCRACKLIB_DICTPATH="\"$(CRACKLIB)\""

#LDAP
LDAP_LIB = -lldap_r -llber
LDAP_INC_PATH = /usr/local/openldap-2.4.47
LDAP_INC = -I$(LDAP_INC_PATH)/include -I$(LDAP_INC_PATH)/servers/slapd -I$(LDAP_INC_PATH)/build-servers/include

#conclude
INCS = $(LDAP_INC) -I$(DBM_INC_PATH) -I$(MYSQL_INC_PATH) -I$(NC_INC_PATH)
LIBS = $(LDAP_LIB) $(CRACKLIB_LIB_FILE) $(MYSQL_LIB_FILE) $(DBM_LIB_FILE) $(NC_LIB_FILE)

bookMan: bookMan.o clishell.o clidb.o ncgui.o socketcli.o libbcrypt/bcrypt.a check_password/check_password.o
	$(CC) $(CFLAGS) $(CRFLAG) $(DFLAGS) -o bookMan $(INCS) $^ $(LIBS) -L$(MYSQL_LIB_PATH) 
.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) $(INCS) -c $<

libbcrypt/bcrypt.a: $(wildcard libbcrypt/*.c libbcrypt/*.h)
	make -C libbcrypt/

check_password/check_password.o: $(wildcard check_password/*.c check_password/*.h)
	make -C check_password/

.PHONY += clean

clean: 
	rm -rf *.o bsdq_cache bookMan;\
	make clean -C libbcrypt/;\
	make clean -C check_password/

