all: gen_aes_key install

LIB = -lcrypto

gen_aes_key: gen_aes_key.o aes_options.o
	gcc -o $@ gen_aes_key.o aes_options.o $(LIB)

.phony += clean

clean: 
	rm -rf *.o gen_aes_key .aes_key.config

install:
	./gen_aes_key;\
	rm -rf gen_aes_key
