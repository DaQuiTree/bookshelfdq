all: clean prepare bsdq_server#srvdbmain #bsdq_client

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

bsdq_server: bsdq_server.o socketsrv.o srvdb.o srvshell.o libbcrypt/bcrypt.a aes/aes_options.o
	$(CC) $(CFLAGS) $(DFLAGS) -o bsdq_server -I$(MYSQL_INC_PATH) $^ $(MYSQL_LIB_FILE) $(AES_LIB_FILE) -L$(MYSQL_LIB_PATH)

.c.o:
	$(CC) $(CFLAGS) $(DFLAGS) -I$(MYSQL_INC_PATH) -c $<

libbcrypt/bcrypt.a:$(wildcard libbcrypt/*.c libbcrypt/*.h)
	make -C libbcrypt/

aes/aes_options.o:$(wildcard aes/*.c aes/*.h)
	make -C aes/

.PHONY += clean

prepare:
	echo $(shell whoami) > .make.config

clean: 
	rm -rf *.o bsdq_server .make.config;\
	make clean -C libbcrypt/;\
	make clean -C aes/

.PHONY += install

cur_user = $(shell cat .make.config)
install:
	mkdir /usr/local/bookshelfdq 2>/dev/null ;\
	mv aes/.aes_key.config /usr/local/bookshelfdq 2>/dev/null;\
	cp bsdq_server /usr/local/bin/;\
	chown -R $(cur_user):$(cur_user) /usr/local/bookshelfdq /usr/local/bin/bsdq_server

.PHONY += uninstall

root_dir=/usr/local/bookshelfdq
server_binpath=/usr/local/bin/bsdq_server
client_binpath=/usr/local/bin/bookMan

uninstall:
	rm -f $(server_binpath);\
	if [ ! -f $(client_binpath) ];\
	then\
		rm -rf $(root_dir);\
	fi
