all:srvdb.o 

CC = gcc
CFLAGS = -Wall -pedantic

#Debug Flags
DFLAGS = -g -DDEBUG_TRACE=1

#MYSQL
MYSQL_INC_PATH = /usr/include/mysql
MYSQL_LIB_PATH = /usr/lib
MYSQL_LIB_FILE = -lmysqlclient

srvdb.o: srvdb.c clisrv.h srvdb.h 
	$(CC) $(CFLAGS) -I$(MYSQL_INC_PATH) -c srvdb.c

.PHONY += clean

clean: 
	rm -rf *.o

