all:clean bsdq_server #srvdbmain #bsdq_client

CC = gcc
CFLAGS = -Wall -pedantic

#Debug Flags
#DFLAGS = -g -DDEBUG_TRACE=1

#MYSQL
MYSQL_INC_PATH = /usr/include/mysql
MYSQL_LIB_PATH = /usr/lib/mysql
MYSQL_LIB_FILE = -lmysqlclient

#OPENSSL
AES_LIB_FILE = -lcrypto

bsdq_server: bsdq_server.o socketsrv.o srvdb.o libbcrypt/bcrypt.a aes/aes_options.o
	$(CC) $(CFLAGS) $(DFLAGS) -o bsdq_server -I$(MYSQL_INC_PATH) $^ $(MYSQL_LIB_FILE) $(AES_LIB_FILE) -L$(MYSQL_LIB_PATH)

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -I$(MYSQL_INC_PATH) -c $<

libbcrypt/bcrypt.a:$(wildcard libbcrypt/*.c libbcrypt/*.h)
	make -C libbcrypt/

aes/aes_options.o:$(wildcard aes/*.c aes/*.h)
	make -C aes/

.PHONY += clean

clean: 
	rm -rf *.o bsdq_server;\
	make clean -C libbcrypt/;\
	make clean -C aes/
