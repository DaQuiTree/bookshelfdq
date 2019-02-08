#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clidb.h"
#include "clisrv.h"

static void showresult(shelf_entry_t *user_shelf);

int main()
{
    shelf_entry_t shelf, newshelf;
    int i;

    if(!clidb_init("daqui")){
        exit(EXIT_FAILURE);
    }
    printf("init ok!\n");

    /*shelf.code = 1;*/
    /*strcpy(shelf.name, "5141书架");*/
    /*shelf.nfloors = 2;*/
    /*shelf.ndepth[0] = 3;*/
    /*shelf.ndepth[1] = 3;*/
    /*strcpy(shelf.building_time, "20190208");*/
    /*if(!clidb_shelf_insert(&shelf)){*/
        /*exit(EXIT_FAILURE);*/
    /*}*/
    /*printf("1 insert ok!\n");*/

    /*shelf.code = 7;*/
    /*strcpy(shelf.name, "8322书架");*/
    /*shelf.nfloors = 3;*/
    /*shelf.ndepth[0] = 3;*/
    /*shelf.ndepth[1] = 3;*/
    /*shelf.ndepth[2] = 2;*/
    /*strcpy(shelf.building_time, "20190208");*/
    /*if(!clidb_shelf_insert(&shelf)){*/
        /*exit(EXIT_FAILURE);*/
    /*}*/
    /*printf("7 insert ok!\n");*/

    /*shelf.code = 15;*/
    /*strcpy(shelf.name, "3536书架");*/
    /*shelf.nfloors = 3;*/
    /*shelf.ndepth[0] = 3;*/
    /*shelf.ndepth[1] = 3;*/
    /*shelf.ndepth[2] = 2;*/
    /*strcpy(shelf.building_time, "20190208");*/
    /*if(!clidb_shelf_insert(&shelf)){*/
        /*exit(EXIT_FAILURE);*/
    /*}*/
    /*printf("15 insert ok!\n");*/

    /*if(clidb_shelf_delete(14)){*/
        /*printf("14 delete ok!\n");*/
    /*}*/

    /*if(clidb_shelf_delete(1)){*/
        /*printf("1 delete ok!\n");*/
    /*}*/

    for(i = 1; i <= MAX_SHELF_NUM; i++){
        if(clidb_shelf_exists(i)){
            if(clidb_shelf_get(i, &newshelf)){
                printf("\n");
                showresult(&newshelf);
            }
        }
    }

}

static void showresult(shelf_entry_t *user_shelf)
{
    int i = 0;

    printf("%d\n", user_shelf->code);
    printf("%s\n", user_shelf->name);
    printf("%d\n", user_shelf->nfloors);

    for(i = 0; i < user_shelf->nfloors; i++)
    {
        printf("%d ", user_shelf->ndepth[i]);
    }
    printf("\n");
    printf("%s\n", user_shelf->building_time);
}
