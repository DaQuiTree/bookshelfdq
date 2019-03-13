all:bsdq_server #srvdbmain  #bsdq_client

CC = gcc
CFLAGS = -Wall -pedantic

#Debug Flags
DFLAGS = -g -DDEBUG_TRACE=1

#MYSQL
MYSQL_INC_PATH = /usr/include/mysql
MYSQL_LIB_PATH = /usr/lib/mysql
MYSQL_LIB_FILE = -lmysqlclient

bsdq_server: bsdq_server.o socketsrv.o srvdb.o libbcrypt/bcrypt.a
	$(CC) $(CFLAGS) $(DFLAGS) -o bsdq_server -I$(MYSQL_INC_PATH) srvdb.o socketsrv.o bsdq_server.o libbcrypt/bcrypt.a $(MYSQL_LIB_FILE) -L$(MYSQL_LIB_PATH)

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -I$(MYSQL_INC_PATH) -c $<

libbcrypt/bcrypt.a:$(wildcard libbcrypt/*.c libbcrypt/*.h)
	make -C libbcrypt/

.PHONY += clean

clean: 
	rm -rf *.o bsdq_server;\
	make clean -C libbcrypt/

