#include <stdlib.h>
#include <stdio.h>

#include "mysql.h"

void display_row(void);

MYSQL my_connection;
MYSQL_ROW sqlrow;
int count;

MYSQL_RES *store_result(void);
MYSQL_RES * find_result(void);
int use_result(MYSQL_RES *res_p);

int main(int argc, char *argv[]) {
   int res;
   MYSQL_RES *res_ptr;

   mysql_init(&my_connection);  
   if (mysql_real_connect(&my_connection, "localhost", "bsdq", "bsdq", "foo", 0, NULL, CLIENT_FOUND_ROWS)) {
      if ( mysql_set_character_set( &my_connection, "utf8" ) ) {
            fprintf ( stderr , "错误, %s/n" , mysql_error( &my_connection) ) ;
        }
      printf("Connection success\n");

      res_ptr = find_result();
      count = mysql_field_count(&my_connection);
      res = mysql_query(&my_connection, "SELECT * FROM daqui_books WHERE bookno > 108");
      if (!res_ptr) {
      }else{
          while(use_result(res_ptr))
          {
            printf("Fetched data...\n");
          }
          mysql_free_result(res_ptr);
      }
      mysql_close(&my_connection);
   } else {
      fprintf(stderr, "Connection failed\n");
      if (mysql_errno(&my_connection)) {
          fprintf(stderr, "Connection error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
      }
   }

   return EXIT_SUCCESS;
}

MYSQL_RES *find_result(void)
{
    int res;
    MYSQL_RES *res_p;

    res = mysql_query(&my_connection, "SELECT * FROM daqui_books WHERE bookno > 108");
    if(res){
        return NULL;
    }else{
        return(mysql_store_result(&my_connection));
    }
}

int use_result(MYSQL_RES *res_p)
{
      sqlrow = mysql_fetch_row(res_p);
      if (mysql_errno(&my_connection)){
          fprintf(stderr, "Retrive error: %s\n", mysql_error(&my_connection));
          return 0;
      }
      if (sqlrow == NULL)return 0;
      display_row();
      return 1;
}

void display_row(void)
{
    unsigned int field_count;

    field_count = 0;
    while (field_count < count) {
        printf("%s ", sqlrow[field_count++]);
    }

    printf("\n");
}
