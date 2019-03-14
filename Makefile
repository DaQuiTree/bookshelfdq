all: bsdqclient/bookMan bsdqserver/bsdq_server

bsdqclient/bookMan:
	make -C bsdqclient/

bsdqserver/bsdq_server:
	make -C bsdqserver/

.phony += clean

clean:
	make clean -C bsdqclient/;\
	make clean -C bsdqserver/
