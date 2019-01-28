#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql.h>
#include "clisrv.h"


static MYSQL my_connection;
static const char *my_user = "bsdq";
static const char *my_passwd = "bsdq";
static const char *my_bsdq_db = "bsdq_db";

static int get_simple_result(MYSQL *conn, char* res_str) 
{
    MYSQL_RES *res_ptr;
    MYSQL_ROW sqlrow;

    res_ptr = mysql_use_result(conn);
    if (res_ptr){
        sqlrow = mysql_fetch_row(res_ptr);
        if (sqlrow[0] != NULL){
            strcpy(res_str, sqlrow[0]);
            mysql_free_result(res_ptr);
            return 1;
        }
    }
    mysql_free_result(res_ptr);
    res_str = NULL;
    return 0;
}

int srvdb_init(void)
{
    if (mysql_init(&my_connection) == NULL)
    {
#if DEBUG_TRACE
        fprintf(stderr, "mysql init error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
    return(1);
}

int srvdb_connect(const char* hostname, unsigned int port)
{
    if (!mysql_real_connect(&my_connection, hostname, my_user, my_passwd, my_bsdq_db, port, NULL, 0))
    {
#if DEBUG_TRACE
        fprintf(stderr, "mysql connect error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
    return(1);
}

int srvdb_user_archive_init(const char* username)
{
    char table_name[128];
    char qs[1024];
    int result;

    sprintf(table_name, "%s_shelves", username);
    sprintf(qs, "CREATE TABLE IF NOT EXISTS %s(\
                shelfno INT PRIMARY KEY NOT NULL,\
                name VARCHAR(200) NOT NULL,\
                nfloors INT NOT NULL,\
                floor_depth VARCHAR(20) NOT NULL,\ 
                building_time VARCHAR(80))", table_name);
    result = mysql_query(&my_connection, qs);
    if (result){
#if DEBUG_TRACE
        sprintf("init table:%s error %d: %s\n", table_name, mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
    sprintf(table_name, "%s_books", username);
    sprintf(qs, "CREATE TABLE IF NOT EXISTS %s(\
                shelfno INT NOT NULL,\ 
                floorno INT NOT NULL,\
                bookno INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\
                name VARCHAR(200) NOT NULL,\
                author VARCHAR(70),\
                label VARCHAR(200),\
                borrowed CHAR default '0',\
                on_reading CHAR default '0',\
                cleaned CHAR default '0',\
                encoding_time VARCHAR(80))", table_name);
    result = mysql_query(&my_connection, qs);
    if (result){
#if DEBUG_TRACE
        sprintf("init table:%s error %d: %s\n", table_name, mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
    return(1);
}

int srvdb_book_insert(message_cs_t *msg)
{
    char table_name[128];
    char es_name[BOOK_NAME_LEN+1];
    char es_author[AUTHER_NAME_LEN+1];
    char es_label[MAX_LABEL_NUM*LABEL_NAME_LEN+1];
    char is[1024];
    char temp_str[512];
    int res, bookno_used;

    if (msg->user = NULL){
#if DEBUG_TRACE
        fprintf(stderr, "insert book error: user undefined.\n");
#endif
        return(0);
    }

    sprintf(table_name, "%s_books", msg->user);
    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->book.name, strlen(msg->book.name));
    mysql_escape_string(es_author, msg->book.author, strlen(msg->book.name));
    mysql_escape_string(es_label, msg->book.label, strlen(msg->book.name));

    //查找最小的可用的书籍编号
    sprintf(is, "SELECT MIN(bookno) FROM %s WHERE cleaned=1", table_name);
    res = mysql_query(&my_connection, is);//在废弃编号里查找
    if(!res) {
       if (get_simple_result(&my_connection, temp_str)){
            if(temp_str != NULL){ //有废弃的可用编号
                sscanf(temp_str, "%d", &bookno_used);
                sprintf(is, "UPDATE %s SET cleaned=%d,shelfno=%d,floorno=%d,name=%s,author=%s,\
                        label=%s,borrowed=%d,on_reading=%d,encoding_time=%s WHERE bookno=%d",\
                        table_name, 0, (unsigned int)msg->stuff.book.code[0], (unsigned int)msg->stuff.book.code[1],\
                        es_name, es_author, es_label, msg->stuff.book.borrowed, msg->stuff.book.on_reading,\ 
                        msg->stuff.book.encoding_time, bookno_used);
                res = mysql_query(&my_connection, is);
                if(!res){
                    if(my_sql_affected_rows(&my_connection) == 1){
                        msg->stuff.book.code[2] = (qbyte_t)(bookno_used/256);
                        msg->stuff.book.code[3] = (qbyte_t)(bookno_used%256);
                        return(1);
                    }
                }else{
#if DEBUG_TRACE
                    fprintf(stderr, "Insert book UPDATE error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
                }
            }
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "SELECT MIN(bookno) error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    }

    //直接插入并自动分配编号
    sprintf(is, "SELECT MAX(bookno) FROM %s", table_name);
    res = mysql_query(&my_connection, is);//查看编号是否超出最大值
    if(!res) {
        if(get_simple_result(&my_connection, temp_str));
            if(temp_str != NULL){
                sscanf(sqlrow[0], "%d", &bookno_used);
                if(bookno_used >= MAX_BOOK_NUM){
                    sprintf(msg->error_test, "Insert book failed: book number reached MAX.");
                    return(0);
                }
            }
        }else{
#if DEBUG_TRACE
        fprintf(stderr, "SELECT MAX(bookno)error: get_simple_result()\n");
#endif
            return(0);
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "SELECT MAX(bookno) error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
         
    sprintf(is, "INSERT INTO %s(shelfno, floorno, name, author, label, borrowed, on_reading, cleaned, encoding_time)\
            VALUES(%d, %d, %s, %s, %s, %d, %d, %d, %s)", table_name, (unsigned int)msg->stuff.book.code[0], \
            (unsigned int)msg->stuff.book.code[1], es_name, es_author, es_label, msg->stuff.book.borrowed, \
            msg->stuff.book.on_reading, 0, msg->stuff.book.encoding_time);
    res = mysql_query(&my_connection, is);//查看编号是否超出最大值
    if(!res)return(1);
#if DEBUG_TRACE
        fprintf(stderr, "INSERT INTO book error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);
}

int srvdb_book_delete(message_cs_t *msg)
{
    char table_name[128];
    char is[1024];
    int bookno_del;

    sprintf(table_name, "%s_books", msg->user);
    bookno_del = msg->code[2]*256+msg->code[3];
    
    sprintf(is, "UPDATE %s SET cleaned=1 WHERE bookno=%d", bookno_del);
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
        fprintf(stderr, "Delete bookno %d error %d: %s\n", bookno_del, mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);
}

int srvdb_book_update(message_cs_t *msg)
{
    char table_name[128];
    char es_name[BOOK_NAME_LEN+1];
    char es_author[AUTHER_NAME_LEN+1];
    char es_label[MAX_LABEL_NUM*LABEL_NAME_LEN+1];
    char is[1024];
    int bookno_update;

    sprintf(table_name, "%s_books", msg->user);
    bookno_update = msg->code[2]*256+msg->code[3];
    
    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->book.name, strlen(msg->book.name));
    mysql_escape_string(es_author, msg->book.author, strlen(msg->book.name));
    mysql_escape_string(es_label, msg->book.label, strlen(msg->book.name));

    sprintf(is, "UPDATE %s SET  shelfno=%d, floorno=%d, name=%s, author=%s,\
            label=%s, borrowed=%d, on_reading=%d, cleaned=%d, encoding_time=%s WHERE bookno=%d",\
            table_name, (unsigned int)msg->stuff.book.code[0], (unsigned int)msg->stuff.book.code[1],\
            es_name, es_author, es_label, msg->stuff.book.borrowed, msg->stuff.book.on_reading,\ 
            msg->stuff.book.cleaned, msg->stuff.book.encoding_time, bookno_update);
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
        fprintf(stderr, "Update bookno %d error %d: %s\n", bookno_update,\
                mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);
}

int srvdb_book_find(message_cs_t *msg)
{
    char table_name[128];
    char es_name[BOOK_NAME_LEN+1];
    char es_author[AUTHER_NAME_LEN+1];
    char es_label[MAX_LABEL_NUM*LABEL_NAME_LEN+1];
    char is[1024];

    sprintf(table_name, "%s_books", msg->user);
    bookno_update = msg->code[2]*256+msg->code[3];
    
    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->book.name, strlen(msg->book.name));
    mysql_escape_string(es_author, msg->book.author, strlen(msg->book.name));
    mysql_escape_string(es_label, msg->book.label, strlen(msg->book.name));
}
int srvdb_book_get(message_cs_t *msg);

int srvdb_shelf_insert(message_cs_t *msg);
int srvdb_shelf_get(message_cs_t *msg);
int srvdb_shelf_delte(message_cs_t *msg);
int srvdb_shelf_update(message_cs_t *msg);
int srvdb_shelf_find(message_cs_t *msg);

