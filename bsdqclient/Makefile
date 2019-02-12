all: testshell #test_clidb bsdq_server bsdq_client srvdbmain

CC = gcc
CFLAGS = -Wall -pedantic

#Debug Flags
DFLAGS = -g -DDEBUG_TRACE=1

#MYSQL
MYSQL_INC_PATH = /usr/include/mysql
MYSQL_LIB_PATH = /usr/lib/mysql
MYSQL_LIB_FILE = -lmysqlclient

#DBM
DBM_INC_PATH = /usr/include/gdbm
DBM_LIB_FILE = -lgdbm_compat -lgdbm

#NCURSES
NC_LIB_FILE = -lncursesw


srvdbmain: srvdb.o srvdbmain.o
	$(CC) $(CFLAGS) $(DFLAGS) -o srvdbmain -I$(MYSQL_INC_PATH)  $^ $(MYSQL_LIB_FILE) -L$(MYSQL_LIB_PATH) 
bsdq_server: bsdq_server.o socketcom.o srvdb.o
	$(CC) $(CFLAGS) $(DFLAGS) -o bsdq_server -I$(MYSQL_INC_PATH) $^ $(MYSQL_LIB_FILE) -L$(MYSQL_LIB_PATH)
bsdq_client: bsdq_client.o socketcom.o srvdb.o
	$(CC) $(CFLAGS) $(DFLAGS) -o bsdq_client -I$(MYSQL_INC_PATH) $^  $(MYSQL_LIB_FILE) -L$(MYSQL_LIB_PATH)
test_clidb: test_clidb.o clidb.o
	$(CC) $(CFLAGS) $(DFLAGS) -o test_clidb -I$(DBM_INC_PATH) $^ $(DBM_LIB_FILE)
testshell: testshell.o clishell.o clidb.o ncgui.o ../bsdqserver/socketcom.o ../bsdqserver/srvdb.o
	$(CC) $(CFLAGS) $(DFLAGS) -o testshell -I$(DBM_INC_PATH) -I$(MYSQL_INC_PATH) $^ $(MYSQL_LIB_FILE) $(DBM_LIB_FILE) $(NC_LIB_FILE) -L$(MYSQL_LIB_PATH) 

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -c $<

srvdb.o: srvdb.c clisrv.h srvdb.h 
	$(CC) $(CFLAGS) $(DFLAGS) -I$(MYSQL_INC_PATH) -c $<

#socketcom.o: socketcom.c socketcom.h clisrv.h srvdb.h 
#srvdbmain.o: srvdbmain.c clisrv.h srvdb.h
#bsdq_server.o: bsdq_server.c socketcom.h clisrv.h srvdb.h
#bsdq_client.o: bsdq_client.c socketcom.h clisrv.h
#clidb.o:clidb.c clisrv.h clidb.h
#test_clidb.o: test_clidb.c clisrv.h clidb.h
#clishell.o: clishell.c clisrv.h clidb.h clishell.h

.PHONY += clean

clean: 
	rm -rf *.o bsdq_server bsdq_client srvdbmain test_clidb testshell

