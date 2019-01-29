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
    res_str[0] = '\0';
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
                shelfno INT PRIMARY KEY NOT NULL AUTO_INCREMENT,\
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
    int res, bookno_used, shelfno_save, floorno_save;

    if (msg->user[0] = '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "insert book error: user undefined.\n");
#endif
        return(0);
    }

    sprintf(table_name, "%s_books", msg->user);
    shelfno_save = msg->stuff.book.code[0];
    floorno_save = msg->stuff.book.code[1];
    
    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->stuff.book.name, strlen(msg->stuff.book.name));
    mysql_escape_string(es_author, msg->stuff.book.author, strlen(msg->stuff.book.author));
    mysql_escape_string(es_label, msg->stuff.book.label, strlen(msg->stuff.book.label));

    //查找最小的可用的书籍编号
    sprintf(is, "SELECT MIN(bookno) FROM %s WHERE cleaned=%d", table_name, BOOK_DEL);
    res = mysql_query(&my_connection, is);//在废弃编号里查找
    if(!res) {
       if (get_simple_result(&my_connection, temp_str)){
            if(temp_str != NULL){ //有废弃的可用编号
                sscanf(temp_str, "%d", &bookno_used);
                sprintf(is, "UPDATE %s SET cleaned=%d,shelfno=%d,floorno=%d,name='%s',author='%s',\
                        label='%s',borrowed=%d,on_reading=%d,encoding_time='%s' WHERE bookno=%d",\
                        table_name, BOOK_AVL, shelfno_save, floorno_save, es_name, es_author, es_label,\
                        msg->stuff.book.borrowed, msg->stuff.book.on_reading, msg->stuff.book.encoding_time, bookno_used);
                res = mysql_query(&my_connection, is);
                if(!res){
                    if(my_sql_affected_rows(&my_connection) == 1){
                        msg->stuff.book.code[2] = bookno_used;
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
            if(temp_str[0] != '\0'){
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
            VALUES(%d, %d, '%s', '%s', '%s', %d, %d, %d, '%s')", table_name, shelfno_save, \
            floorno_save, es_name, es_author, es_label, msg->stuff.book.borrowed, \
            msg->stuff.book.on_reading, BOOK_AVL, msg->stuff.book.encoding_time);
    res = mysql_query(&my_connection, is);
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
    bookno_del = msg->stuff.book.code[2];
    
    sprintf(is, "UPDATE %s SET cleaned=%d WHERE bookno=%d", table_name, BOOK_DEL, bookno_del);
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
    int  shelfno_save, floorno_save, bookno_update;

    sprintf(table_name, "%s_books", msg->user);
    shelfno_save = msg->stuff.book.code[0];
    floorno_save = msg->stuff.book.code[1];
    bookno_update = msg->stuff.book.code[2];
    
    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->stuff.book.name, strlen(msg->stuff.book.name));
    mysql_escape_string(es_author, msg->stuff.book.author, strlen(msg->stuff.book.author));
    mysql_escape_string(es_label, msg->stuff.book.label, strlen(msg->stuff.book.label));

    sprintf(is, "UPDATE %s SET  shelfno=%d, floorno=%d, name='%s', author='%s',\
            label='%s', borrowed=%d, on_reading=%d, cleaned=%d, encoding_time='%s' WHERE bookno=%d",\
            table_name, shelfno_save, floorno_save,es_name, es_author, es_label, msg->stuff.book.borrowed,\
            msg->stuff.book.on_reading, msg->stuff.book.cleaned, msg->stuff.book.encoding_time, bookno_update);
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
        fprintf(stderr, "Update bookno %d error %d: %s\n", bookno_update,\
                mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);
}

int srvdb_book_find(message_cs_t *msg, MYSQL_RES *res_ptr)
{
    char table_name[128];
    char is[1024];
    int shelfno_save, floorno_save, bookno_start;
    char es_name_sql[BOOK_NAME_LEN+32+1]="";
    char es_author_sql[AUTHER_NAME_LEN+32+1]="";
    char es_label_sql[MAX_LABEL_NUM*LABEL_NAME_LEN+32+1]="";
    char es_temp[BOOK_NAME_LEN+1];
    char floor_sql[32]="";

    sprintf(table_name, "%s_books", msg->user);
    shelfno_save = msg->stuff.book.code[0];
    floorno_save = msg->stuff.book.code[1];
    bookno_start = msg->stuff.book.code[2];

    char symbol = '=';
    if(shelfno == NON_SENSE_INT) symbol = '>';//全局搜索?
    if (msg->stuff.book.cleaned == BOOK_UNDEF)
    {   //查询未归档的图书
        sprintf(is, "SELECT * FROM %s WHERE cleaned=%d and shelfno%c%d and bookno>%d ORDER BY bookno LIMIT %d",\ 
                table_name, BOOK_UNDEF, symbol, shelfno_save, bookno_start, DEFAULT_FINDS); 
    }else(msg->stuff.book.cleaned == BOOK_AVL){
        if (msg->stuff.book.borrowed == FIND_FLAG_INT){
            //查询外借图书
            sprintf(is, "SELECT * FROM %s WHERE borrowed=1 and cleaned=%d and shelfno%c%d and bookno>%d ORDER BY bookno LIMIT %d",\ 
                    table_name, BOOK_AVL, symbol, shelfno_save, bookno_start, DEFAULT_FINDS); 
        }else if(msg->stuff.book.on_reading == FIND_FLAG_INT){
            //查询在读图书
            sprintf(is, "SELECT * FROM %s WHERE on_reading=1 and cleaned=%d and shelfno%c%d and bookno>%d ORDER BY bookno LIMIT %d",\ 
                    table_name, BOOK_AVL, symbol, shelfno_save, bookno_start, DEFAULT_FINDS); 
        }else{
            //关键字检索图书
            if (bookno_start != NON_SENSE_INT && bookno_start < 0){
                //指定书号查询
                sprintf(is, "SELECT * FROM %s WHERE bookno=%d", table_name, -bookno_start);
            }else{
                //书名[0]：'-'模糊查找,'+'半精确查找,'='精确查找
                mysql_escape_string(es_temp, msg->stuff.book.name+1, strlen(msg->stuff.book.name)-1);//保护字符串中的特殊字符
                switch (msg->stuff.book.name[0])
                {
                    case '0':
                    case '-':
                    case '+':
                        sprintf(es_name_sql, " AND (name LIKE '%%%s%%' ", es_temp);
                        break; 
                    case '=':
                        sprintf(es_name_sql, " AND (name='%s' ", es_temp);
                        break; 
                    default:
#if DEBUG_TRACE
                        fprintf(stderr, "find() name search str format error.\n");
#endif
                        return(-1);
                        break;
                }

                //作者[0]：'-'模糊查找,'+'半精确查找,'='精确查找
                mysql_escape_string(es_temp, msg->stuff.book.author+1, strlen(msg->stuff.book.author)-1);
                switch (msg->stuff.book.author[0])
                {
                    case '-':
                        sprintf(es_author_sql, " OR author LIKE '%%%s%%' ", es_temp);
                        break;
                    case '0':
                    case '+':
                        sprintf(es_author_sql, " AND author LIKE '%%%s%%' ", es_temp);
                        break; 
                    case '=':
                        sprintf(es_author_sql, " AND author='%s' ", es_temp);
                        break; 
                    default:
#if DEBUG_TRACE
                        fprintf(stderr, "find() author search str format error.\n");
#endif
                        return(-1);
                        break;
                }

                //标签[0]：'-'模糊查找,'+'半精确查找,'='精确查找
                mysql_escape_string(es_temp, msg->stuff.book.label+1, strlen(msg->stuff.book.label)-1);
                switch (msg->stuff.book.label[0])
                {
                    case '-':
                        sprintf(es_label_sql, " OR label LIKE '%%%s%%') ", es_temp);
                        break;
                    case '0':
                    case '+':
                        sprintf(es_label_sql, " AND label LIKE '%%%s%%') ", es_temp);
                        break; 
                    case '=':
                        sprintf(es_label_sql, " AND label='%s') ", es_temp);
                        break; 
                    default:
#if DEBUG_TRACE
                        fprintf(stderr, "find() label search str format error.\n");
#endif
                        return(-1);
                        break;
                }

                //匹配书架位置
                if (floorno_save != NON_SENSE_INT && shelfno_save != NON_SENSE_INT)sprintf(floor_sql, " and floorno=%d ", floorno_save);//按层检索时要保证指定了书架

                //生产SQL语句
                sprintf(is, "SELECT * FROM %s WHERE bookno>%d %s%s%s%s ORDER BY bookno LIMIT %d",\
                        table_name, bookno_start, floor_sql, es_name_sql, es_author_sql, es_label_sql, DEFAULT_FINDS);
            }
        }
    }

    //检索
    int res;
    int num_rows;
    
    res = mysql_query(&my_connection, is);
    if (res){
#if DEBUG_TRACE
        fprintf(stderr, "find() mysql_query error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    }else{
        res_ptr = mysql_store_result(&my_connection);
        num_rows = mysql_num_rows(res_ptr);
        return(num_rows); 
    }
    return(-1);
}

int srvdb_book_get(message_cs_t *msg);

int srvdb_shelf_insert(message_cs_t *msg);
int srvdb_shelf_get(message_cs_t *msg);
int srvdb_shelf_delte(message_cs_t *msg);
int srvdb_shelf_update(message_cs_t *msg);
int srvdb_shelf_find(message_cs_t *msg);

