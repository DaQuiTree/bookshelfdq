#!/bin/sh

current_path=/home/daqui/GitProj/bookshelfdq/bsdqserver
bin_path=${current_path}/bsdq_server

pgrep bsdq_server
if [ $? = 1 ]; then
    $bin_path -p bsdq
fi

exit
