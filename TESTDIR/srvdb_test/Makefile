all: srvdbmain 

CC = gcc
CFLAGS = -Wall -pedantic

#Debug Flags
DFLAGS = -g -DDEBUG_TRACE=1

#MYSQL
MYSQL_INC_PATH = /usr/include/mysql
MYSQL_LIB_PATH = /usr/lib/mysql
MYSQL_LIB_FILE = -lmysqlclient

srvdbmain: srvdb.o srvdbmain.o
	$(CC) $(CFLAGS) $(DFLAGS) -o srvdbmain -I$(MYSQL_INC_PATH) srvdbmain.o srvdb.o $(MYSQL_LIB_FILE) -L$(MYSQL_LIB_PATH) 

srvdb.o: srvdb.c clisrv.h srvdb.h 
	$(CC) $(CFLAGS) $(DFLAGS) -I$(MYSQL_INC_PATH) -c srvdb.c
srvdbmain.o: srvdbmain.c clisrv.h srvdb.h

.PHONY += clean

clean: 
	rm -rf *.o srvdbmain

