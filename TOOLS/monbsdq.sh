#!/bin/sh

current_path=/usr/local/bookshelfdq
bin_path=${current_path}/bsdq_server

pgrep bsdq_server
if [ $? = 1 ]; then
    $bin_path -p xxxx
fi

exit
