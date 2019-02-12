#include <stdio.h>
#include <stdlib.h>
#include "clishell.h"

int main()
{
    if(!client_initialize("127.0.0.1", "daqui")){
        fprintf(stderr, "client initialize failed\n");
        exit(EXIT_FAILURE);
    }
    if(!client_shelves_info_sync()){
        fprintf(stderr, "get client shelve info failed\n");
        exit(EXIT_FAILURE);
    }
    if(!client_books_info_sync()){
        fprintf(stderr, "get client books info failed\n");
        exit(EXIT_FAILURE);
    }

    client_start_gui();

    exit(EXIT_SUCCESS);
}
