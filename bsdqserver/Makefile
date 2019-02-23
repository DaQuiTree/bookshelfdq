all: bsdq_server #bsdq_client #srvdbmain

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
bsdq_server: bsdq_server.o socketcom.o srvdb.o
	$(CC) $(CFLAGS) $(DFLAGS) -o bsdq_server -I$(MYSQL_INC_PATH) srvdb.o socketcom.o bsdq_server.o $(MYSQL_LIB_FILE) -L$(MYSQL_LIB_PATH)
bsdq_client: bsdq_client.o socketcom.o srvdb.o
	$(CC) $(CFLAGS) $(DFLAGS) -o bsdq_client -I$(MYSQL_INC_PATH) srvdb.o socketcom.o bsdq_client.o $(MYSQL_LIB_FILE) -L$(MYSQL_LIB_PATH)

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -I$(MYSQL_INC_PATH) -c $<

srvdb.o: srvdb.c clisrv.h srvdb.h 
socketcom.o: socketcom.c socketcom.h clisrv.h srvdb.h
srvdbmain.o: srvdbmain.c clisrv.h srvdb.h
bsdq_server.o: bsdq_server.c socketcom.h clisrv.h srvdb.h
bsdq_client.o: bsdq_client.c socketcom.h clisrv.h

.PHONY += clean

clean: 
	rm -rf *.o bsdq_server bsdq_client srvdbmain

