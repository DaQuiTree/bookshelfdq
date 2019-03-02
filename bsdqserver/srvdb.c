#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mysql.h>
#include "srvdb.h"
#include "clisrv.h"

static MYSQL my_connection;
//和检索相关静态变量
static MYSQL_RES *res_ptr = NULL;
static int res_fields = 0;
static int res_rows = 0;

static const char *my_user = "bsdq";
static const char *my_passwd = "bsdq";
static const char *my_bsdq_db = "bsdq_db";
static const char *my_bsdq_accounts = "bsdq_accounts";

static int get_simple_result(MYSQL *conn, char* res_str) 
{
    MYSQL_RES *result_ptr;
    MYSQL_ROW sqlrow; 

    result_ptr = mysql_use_result(conn);
    if (result_ptr){
        sqlrow = mysql_fetch_row(result_ptr);
        if (sqlrow != NULL){
            if(sqlrow[0] != NULL){
                strcpy(res_str, sqlrow[0]);
                mysql_free_result(result_ptr);
                return 1;
            }
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "simple_get() error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    }
    mysql_free_result(result_ptr);
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
    if (mysql_set_character_set(&my_connection, "utf8")){
#if DEBUG_TRACE
        fprintf(stderr, "mysql set utf8 error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    }
    return(1);
}

int srvdb_accounts_table_init(void)
{
    char qs[1024];
    int result;

    sprintf(qs, "CREATE TABLE IF NOT EXISTS %s(\
                userno INT NOT NULL AUTO_INCREMENT,\
                name VARCHAR(200) NOT NULL,\
                password VARCHAR(200) NOT NULL,\
                type INT NOT NULL,\
                PRIMARY KEY (userno, name))", my_bsdq_accounts);
    result = mysql_query(&my_connection, qs);
    if (result){
#if DEBUG_TRACE
        sprintf("init table:%s error %d: %s\n", my_bsdq_accounts, mysql_errno(&my_connection), mysql_error(&my_connection));
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
                nfloors CHAR NOT NULL,\
                floor_depth VARCHAR(50) NOT NULL,\
                cleaned CHAR NOT NULL default '0',\
                building_time VARCHAR(20))", table_name);
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
                encoding_time VARCHAR(20))", table_name);
    result = mysql_query(&my_connection, qs);
    if (result){
#if DEBUG_TRACE
        sprintf("init table:%s error %d: %s\n", table_name, mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
    return(1);
}

//
//书籍信息相关的mysql封装
//
int srvdb_book_insert(message_cs_t *msg)
{
    char table_name[128];
    char es_name[BOOK_NAME_LEN*2+1];
    char es_author[AUTHOR_NAME_LEN*2+1];
    char es_label[(MAX_LABEL_NUM*(LABEL_NAME_LEN+1))*2+1];
    char is[1024];
    char temp_str[512];
    int res, bookno_used = 0, shelfno_save, floorno_save;

    if (msg->user[0] == '\0'){
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
            if(temp_str[0] != '\0'){ //有废弃的可用编号
                sscanf(temp_str, "%d", &bookno_used);
                sprintf(is, "UPDATE %s SET cleaned=%d,shelfno=%d,floorno=%d,name='%s',author='%s',\
                        label='%s',borrowed=%c,on_reading=%c,encoding_time='%s' WHERE bookno=%d",\
                        table_name, BOOK_AVL, shelfno_save, floorno_save, es_name, es_author, es_label,\
                        msg->stuff.book.borrowed, msg->stuff.book.on_reading, msg->stuff.book.encoding_time, bookno_used);
                res = mysql_query(&my_connection, is);
                if(!res){
                    if(mysql_affected_rows(&my_connection) == 1){
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
        if(get_simple_result(&my_connection, temp_str)){
            if(temp_str[0] != '\0'){
                sscanf(temp_str, "%d", &bookno_used);
                if(bookno_used >= MAX_BOOK_NUM){
                    sprintf(msg->error_text, "上架失败:图书数量达到上限");
                    return(0);
                }
                msg->stuff.book.code[2] = bookno_used+1;
            }
        }else{
#if DEBUG_TRACE
            fprintf(stderr, "SELECT MAX(bookno)error: get_simple_result()\n");
#endif
            //书籍表为空
            msg->stuff.book.code[2] = bookno_used = 1;
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "SELECT MAX(bookno) error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
         
    sprintf(is, "INSERT INTO %s(shelfno, floorno, bookno, name, author, label, borrowed, on_reading, cleaned, encoding_time)\
            VALUES(%d, %d, %d, '%s', '%s', '%s', %c, %c, %d, '%s')", table_name, shelfno_save, \
            floorno_save, bookno_used, es_name, es_author, es_label, msg->stuff.book.borrowed, \
            msg->stuff.book.on_reading, BOOK_AVL, msg->stuff.book.encoding_time);
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
        fprintf(stderr, "INSERT INTO book error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    sprintf(msg->error_text, "上架图书失败...");
    return(0);
}

int srvdb_book_delete(message_cs_t *msg)
{
    char table_name[128];
    char is[1024];
    int res, bookno_del, shelfno_save;

    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "delete book error: user undefined.\n");
#endif
        return(0);
    }

    sprintf(table_name, "%s_books", msg->user);
    shelfno_save = msg->stuff.book.code[0];
    bookno_del = msg->stuff.book.code[2];
    
    if(bookno_del == NON_SENSE_INT && shelfno_save != NON_SENSE_INT)
        sprintf(is, "UPDATE %s SET cleaned=%d WHERE shelfno=%d", table_name, BOOK_DEL, shelfno_save);
    else
        sprintf(is, "UPDATE %s SET cleaned=%d WHERE bookno=%d", table_name, BOOK_DEL, bookno_del);
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
    if(bookno_del != NON_SENSE_INT)
        fprintf(stderr, "Delete bookno %d error %d: %s\n", bookno_del, mysql_errno(&my_connection), mysql_error(&my_connection));
    else
        fprintf(stderr, "Delete books of shelfno %d error %d: %s\n", shelfno_save, mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);
}

int srvdb_book_update(message_cs_t *msg)
{
    char table_name[128];
    char es_name[BOOK_NAME_LEN*2+1];
    char es_author[AUTHOR_NAME_LEN*2+1];
    char es_label[(MAX_LABEL_NUM*(LABEL_NAME_LEN+1))*2+1];
    char is[1024];
    int  res, shelfno_save, floorno_save, bookno_update;

    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "Update book error: user undefined.\n");
#endif
        return(0);
    }

    sprintf(table_name, "%s_books", msg->user);
    shelfno_save = msg->stuff.book.code[0];
    floorno_save = msg->stuff.book.code[1];
    bookno_update = msg->stuff.book.code[2];
    
    if(bookno_update == NON_SENSE_INT && shelfno_save != NON_SENSE_INT){//指定书架上所有图书变为未归档状态
        sprintf(is, "UPDATE %s SET cleaned=%d WHERE shelfno=%d", table_name, BOOK_UNSORTED, shelfno_save);
    }else{//更新指定图书
        //保护字符串中的特殊字符
        mysql_escape_string(es_name, msg->stuff.book.name, strlen(msg->stuff.book.name));
        mysql_escape_string(es_author, msg->stuff.book.author, strlen(msg->stuff.book.author));
        mysql_escape_string(es_label, msg->stuff.book.label, strlen(msg->stuff.book.label));

        sprintf(is, "UPDATE %s SET shelfno=%d, floorno=%d, name='%s', author='%s',\
                label='%s', borrowed=%c, on_reading=%c, cleaned=%d, encoding_time='%s' WHERE bookno=%d",\
                table_name, shelfno_save, floorno_save,es_name, es_author, es_label, msg->stuff.book.borrowed,\
                msg->stuff.book.on_reading, BOOK_AVL, msg->stuff.book.encoding_time, bookno_update);
    }
#if DEBUG_TRACE
                fprintf(stderr, "%s.\n", is);
#endif
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
    if(bookno_update != NON_SENSE_INT)
        fprintf(stderr, "Update bookno %d error %d: %s\n", bookno_update,\
                mysql_errno(&my_connection), mysql_error(&my_connection));
    else
        fprintf(stderr, "Update books of shelfno %d error %d: %s\n", shelfno_save,\
                mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);
}

int srvdb_book_find(message_cs_t *msg, int *num_rows)
{
    char table_name[128];
    char is[1024];
    int shelfno_save, floorno_save, bookno_start, flag_autofree_result = 0;
    char es_name_sql[BOOK_NAME_LEN*2+32+1]="";
    char es_author_sql[AUTHOR_NAME_LEN*2+32+1]="";
    char es_label_sql[(MAX_LABEL_NUM*(LABEL_NAME_LEN+1))*2+32+1]="";
    char es_temp[BOOK_NAME_LEN*2+1];
    char floor_sql[32]="";

    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "find book error: user undefined.\n");
#endif
        return(0);
    }

    sprintf(table_name, "%s_books", msg->user);
    shelfno_save = msg->stuff.book.code[0];
    floorno_save = msg->stuff.book.code[1];
    bookno_start = msg->stuff.book.code[2];

    char symbol = '=';
    if(shelfno_save == NON_SENSE_INT) symbol = '>';//全局搜索?

    int limit = DEFAULT_FINDS; 
    char *columns = "*";
    if(bookno_start == BREAK_LIMIT_INT){ //放开搜索结果数量限制?
        bookno_start = NON_SENSE_INT;
        flag_autofree_result = 1;
        limit = MAX_BOOK_NUM;
        columns = "bookno";
    }
    if (msg->stuff.book.borrowed == FLAG_FIND_UNSORTED && msg->stuff.book.on_reading == FLAG_FIND_UNSORTED)
    {   //查询未归档的图书
        sprintf(is, "SELECT %s FROM %s WHERE cleaned=%d and shelfno%c%d and bookno>%d ORDER BY bookno LIMIT %d",\
                columns, table_name, BOOK_UNSORTED, symbol, shelfno_save, bookno_start, limit); 
    }else{
        if (msg->stuff.book.borrowed == FLAG_BORROWED){//查询外借图书
            sprintf(is, "SELECT %s FROM %s WHERE borrowed=1 and cleaned=%d and shelfno%c%d and bookno>%d ORDER BY bookno LIMIT %d",\
                    columns, table_name, BOOK_AVL, symbol, shelfno_save, bookno_start, limit); 
        }else if(msg->stuff.book.on_reading == FLAG_ON_READING){ //查询在读图书
            sprintf(is, "SELECT %s FROM %s WHERE on_reading=1 and cleaned=%d and shelfno%c%d and bookno>%d ORDER BY bookno LIMIT %d",\
                    columns, table_name, BOOK_AVL, symbol, shelfno_save, bookno_start, limit); 
        }else{
            //关键字检索图书
            if (bookno_start != NON_SENSE_INT && bookno_start < 0){
                //指定书号查询
                sprintf(is, "SELECT %s FROM %s WHERE bookno=%d", columns, table_name, -bookno_start);
            }else{
                //书名[0]：'0'忽略,'-'模糊查找,'+'半精确查找,'='精确查找
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
                        fprintf(stderr, "book_find() name search str format error.\n");
#endif
                        return(0);
                        break;
                }

                //作者[0]：'0'忽略,'-'模糊查找,'+'半精确查找,'='精确查找
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
                        fprintf(stderr, "book_find() author search str format error.\n");
#endif
                        return(0);
                        break;
                }

                //标签[0]：'0'忽略,'-'模糊查找,'+'半精确查找,'='精确查找
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
                        fprintf(stderr, "book_find() label search str format error.\n");
#endif
                        return(0);
                        break;
                }

                //匹配书架位置
                if (floorno_save != NON_SENSE_INT && shelfno_save != NON_SENSE_INT)sprintf(floor_sql, " and floorno=%d ", floorno_save);//按层检索时要保证指定了书架

                //生产SQL语句
                sprintf(is, "SELECT %s FROM %s WHERE shelfno%c%d AND cleaned=%d AND bookno>%d %s%s%s%s ORDER BY bookno LIMIT %d",\
                        columns, table_name, symbol, shelfno_save, BOOK_AVL, bookno_start, floor_sql, es_name_sql, es_author_sql, es_label_sql, limit);
            }
        }
    }

#if DEBUG_TRACE
                fprintf(stderr, "%s.\n", is);
#endif
    //检索
    int res;
    
    res = mysql_query(&my_connection, is);
    if (res){
#if DEBUG_TRACE
        fprintf(stderr, "book_find() mysql_query error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    }else{
        res_ptr = mysql_store_result(&my_connection);
        if(res_ptr){
            *num_rows = mysql_num_rows(res_ptr);
            res_rows = *num_rows;
            res_fields = mysql_field_count(&my_connection);
#if DEBUG_TRACE
            fprintf(stderr, "book_find(): %d rows with %d columns\n", *num_rows, res_fields);
#endif
            if(flag_autofree_result){
                flag_autofree_result = 0;
                srvdb_free_result();
            }
            return(1); 
        }
    }
    return(0);
}

int srvdb_book_fetch_result(message_cs_t *msg)
{
    MYSQL_ROW sqlrow;
    int i = 0;

    if(res_ptr == NULL)return(FETCH_RESULT_END);
    sqlrow = mysql_fetch_row(res_ptr);
    if (mysql_errno(&my_connection)){
#if DEBUG_TRACE
        fprintf(stderr, "Retrive error: %s\n", mysql_error(&my_connection));
#endif
        srvdb_free_result();
        return(FETCH_RESULT_ERR);
    }
    if (sqlrow == NULL){
        if (res_rows == DEFAULT_FINDS)return(FETCH_RESULT_END_MORE); //需要继续查询
        srvdb_free_result();
        return(FETCH_RESULT_END);//查询结束
    } 
    sscanf(sqlrow[i++], "%d", &msg->stuff.book.code[0]);
    sscanf(sqlrow[i++], "%d", &msg->stuff.book.code[1]);
    sscanf(sqlrow[i++], "%d", &msg->stuff.book.code[2]);
    strcpy(msg->stuff.book.name, sqlrow[i++]);
    strcpy(msg->stuff.book.author, sqlrow[i++]);
    strcpy(msg->stuff.book.label, sqlrow[i++]);
    sscanf(sqlrow[i++], "%c", &msg->stuff.book.borrowed);
    sscanf(sqlrow[i++], "%c", &msg->stuff.book.on_reading);
    i++;//跳过cleaned
    strcpy(msg->stuff.book.encoding_time, sqlrow[i++]);
    if (i != res_fields)
    {
#if DEBUG_TRACE
        fprintf(stderr, "fetch() error: res_fields didn't match while copying data\n");
#endif
        srvdb_free_result();
        return(FETCH_RESULT_ERR);
    }
    
    return(FETCH_RESULT_CONT);
}

void srvdb_free_result(void)
{
    mysql_free_result(res_ptr);
    res_ptr = NULL;
    res_fields = 0;
    res_rows = 0;
}

int srvdb_book_count(message_cs_t *msg)
{
    book_count_t bc;

    msg->stuff.book.code[2] = BREAK_LIMIT_INT;//打开限制并自动释放结果
    //在读图书数量
    msg->stuff.book.on_reading = FLAG_ON_READING;
    if(!srvdb_book_find(msg, &bc.books_on_reading)){
#if DEBUG_TRACE
        fprintf(stderr, "srvdb_book_count() error: on_reading\n");
#endif
        return(0);
    }

    //外借图书数量
    msg->stuff.book.on_reading = 0;
    msg->stuff.book.borrowed = FLAG_BORROWED;
    if(!srvdb_book_find(msg, &bc.books_borrowed)){
#if DEBUG_TRACE
        fprintf(stderr, "srvdb_book_count() error: borrowed\n");
#endif
        return(0);
    }

    //未处理图书数量
    msg->stuff.book.borrowed = FLAG_FIND_UNSORTED;
    msg->stuff.book.on_reading = FLAG_FIND_UNSORTED;
    if(!srvdb_book_find(msg, &bc.books_unsorted)){
#if DEBUG_TRACE
        fprintf(stderr, "srvdb_book_count() error: unsorted\n");
#endif
        return(0);
    }

    //图书总数
    msg->stuff.book.borrowed = 0;
    msg->stuff.book.on_reading = 0;
    strcpy(msg->stuff.book.name, "0");
    strcpy(msg->stuff.book.author, "0");
    strcpy(msg->stuff.book.label, "0");
    if(!srvdb_book_find(msg, &bc.books_all)){
#if DEBUG_TRACE
        fprintf(stderr, "srvdb_book_count() error: all\n");
#endif
        return(0);
    }
    //srvdb_free_result();

    bc.books_all += bc.books_unsorted;
    *(book_count_t *)&msg->extra_info = bc;
    return(1);
}

//
// 以下是关于书架的MYSQL封装
//

int srvdb_shelf_build(message_cs_t *msg)
{
    char table_name[128];
    char es_name[SHELF_NAME_LEN*2+1];
    char floor_depth_str[MAX_FLOORS*2+1];
    char is[1024];
    char temp_str[512];
    int res, shelfno_used = 0, i = 0;

    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "insert shelf error: user undefined.\n");
#endif
        return(0);
    }

    sprintf(table_name, "%s_shelves", msg->user);
    for ( i = 0; i < msg->stuff.shelf.nfloors; i++)
    {
        floor_depth_str[2*i] = msg->stuff.shelf.ndepth[i] + '0';
        floor_depth_str[2*i+1] = ','; 
    }
    floor_depth_str[2*i-1] = '\0';
    
    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->stuff.shelf.name, strlen(msg->stuff.shelf.name));

    //查找最小的可用的书架编号
    sprintf(is, "SELECT MIN(shelfno) FROM %s WHERE cleaned=%d", table_name, SHELF_DEL);
    res = mysql_query(&my_connection, is);//在废弃编号里查找
    if(!res) {
       if (get_simple_result(&my_connection, temp_str)){
            if(temp_str[0] != '\0'){ //有废弃的可用编号
                sscanf(temp_str, "%d", &shelfno_used);
                sprintf(is, "UPDATE %s SET cleaned=%d, name='%s', nfloors=%d,\
                        floor_depth='%s', building_time='%s' WHERE shelfno=%d",\
                        table_name, SHELF_AVL, es_name, msg->stuff.shelf.nfloors,\
                        floor_depth_str, msg->stuff.shelf.building_time, shelfno_used);
                res = mysql_query(&my_connection, is);
                if(!res){
                    if(mysql_affected_rows(&my_connection) == 1){
                        msg->stuff.shelf.code = shelfno_used;
                        return(1);
                    }
                }else{
#if DEBUG_TRACE
                    fprintf(stderr, "Insert shelf UPDATE error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
                }
            }
       }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "SELECT MIN(shelfno) error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    }

    //直接插入并自动分配编号
    sprintf(is, "SELECT MAX(shelfno) FROM %s", table_name);
    res = mysql_query(&my_connection, is);//查看编号是否超出最大值
    if(!res) {
        if(get_simple_result(&my_connection, temp_str)){
            if(temp_str[0] != '\0'){
                sscanf(temp_str, "%d", &shelfno_used);
                if(shelfno_used >= MAX_SHELF_NUM){
                    sprintf(msg->error_text, "打造失败:书架达到上限");
                    return(0);
                }
                msg->stuff.shelf.code = shelfno_used+1;
            }
        }else{
#if DEBUG_TRACE
            fprintf(stderr, "SELECT MAX(shelfno)error: get_simple_result()\n");
#endif
            msg->stuff.shelf.code = shelfno_used = 1;
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "SELECT MAX(shelfno) error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
         
    sprintf(is, "INSERT INTO %s(shelfno, name, nfloors, floor_depth, cleaned, building_time)\
            VALUES(%d, '%s', %d, '%s', %d, '%s')", table_name, shelfno_used, es_name, msg->stuff.shelf.nfloors,\
            floor_depth_str, SHELF_AVL, msg->stuff.shelf.building_time);
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
        fprintf(stderr, "INSERT INTO shelf error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    sprintf(msg->error_text, "打造书架失败");
    return(0);
}

int srvdb_shelf_remove(message_cs_t *msg)
{
    char table_name[128];
    char is[1024];
    int res, shelfno_del;

    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "delete shelf error: user undefined.\n");
#endif
        return(0);
    }

    sprintf(table_name, "%s_shelves", msg->user);
    shelfno_del = msg->stuff.shelf.code;
   
    //删除书架
    sprintf(is, "UPDATE %s SET cleaned=%d WHERE shelfno=%d", table_name, SHELF_DEL, shelfno_del);
    res = mysql_query(&my_connection, is);
    if(!res)
    {
        //重置图书
        sprintf(table_name, "%s_books", msg->user);
        sprintf(is, "UPDATE %s SET cleaned=%d WHERE shelfno=%d", table_name, BOOK_UNSORTED, shelfno_del);
        if(!res)return(1);
    }
#if DEBUG_TRACE
        fprintf(stderr, "Delete shelfno %d error %d: %s\n", shelfno_del, mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);

}

int srvdb_shelf_update(message_cs_t *msg)
{
    char table_name[128];
    char es_name[BOOK_NAME_LEN*2+1];
    char is[1024];
    char floor_depth_str[MAX_FLOORS*2+1];
    int  res, i, shelfno_update;
 
    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "update shelf error: user undefined.\n");
#endif
        return(0);
    }

    for ( i = 0; i < msg->stuff.shelf.nfloors; i++)
    {
        floor_depth_str[2*i] = msg->stuff.shelf.ndepth[i] + '0';
        floor_depth_str[2*i+1] = ','; 
    }
    floor_depth_str[2*i-1] = '\0';

    sprintf(table_name, "%s_shelves", msg->user);
    shelfno_update = msg->stuff.shelf.code;
    
    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->stuff.shelf.name, strlen(msg->stuff.shelf.name));

    sprintf(is, "UPDATE %s SET name='%s', nfloors=%d, floor_depth='%s',\
            building_time='%s' WHERE shelfno=%d", table_name, es_name,\
            msg->stuff.shelf.nfloors, floor_depth_str, msg->stuff.shelf.building_time, shelfno_update);
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
    fprintf(stderr, "Update shelfno %d error %d: %s\n", shelfno_update,\
            mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);
}

int srvdb_shelf_find(message_cs_t *msg, int *num_rows)
{
    char table_name[128];
    char is[1024];
    int shelfno_find;

    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "find shelf error: user undefined.\n");
#endif
        return(0);
    }
    sprintf(table_name, "%s_shelves", msg->user);
    shelfno_find = msg->stuff.shelf.code; 

    int limit = DEFAULT_FINDS;
    char *columns = "*";
    if (shelfno_find == BREAK_LIMIT_INT){
        shelfno_find = 0;
        limit = MAX_SHELF_NUM;
        columns = "shelfno";
    }

    if(shelfno_find < 0){
        sprintf(is, "SELECT %s FROM %s WHERE shelfno=%d AND cleaned=%d ORDER BY shelfno LIMIT %d", columns, table_name, -shelfno_find, SHELF_AVL, limit); //查找指定书架信息
    }else{
        sprintf(is, "SELECT %s FROM %s WHERE shelfno>%d AND cleaned=%d ORDER BY shelfno LIMIT %d", columns, table_name, shelfno_find, SHELF_AVL, limit); //查找所有书架信息
    }
    
    int res;
    
    res = mysql_query(&my_connection, is);
    if (res){
#if DEBUG_TRACE
        fprintf(stderr, "shelf_find() mysql_query error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    }else{
        res_ptr = mysql_store_result(&my_connection);
        if(res_ptr){
            *num_rows = mysql_num_rows(res_ptr);
            res_rows = *num_rows;
            res_fields = mysql_field_count(&my_connection);
#if DEBUG_TRACE
            fprintf(stderr, "shelf_find(): %d rows with %d columns\n", *num_rows, res_fields);
#endif
            return(1); 
        }
    }
    return(0);
}

int srvdb_shelf_fetch_result(message_cs_t *msg)
{
    MYSQL_ROW sqlrow;
    int i = 0, j = 0;

    sqlrow = mysql_fetch_row(res_ptr);
    if (mysql_errno(&my_connection)){
#if DEBUG_TRACE
        fprintf(stderr, "shelf Retrive error: %s\n", mysql_error(&my_connection));
#endif
        srvdb_free_result();
        return(FETCH_RESULT_ERR);
    }
    if (sqlrow == NULL){
        if (res_rows == DEFAULT_FINDS)return(FETCH_RESULT_END_MORE); //需要继续查询
        srvdb_free_result();
        return(FETCH_RESULT_END);//查询结束
    } 
    sscanf(sqlrow[i++], "%d", &msg->stuff.shelf.code);
    strcpy(msg->stuff.shelf.name, sqlrow[i++]);
    msg->stuff.shelf.nfloors = sqlrow[i++][0] - '0';
    for (j = 0; j < msg->stuff.shelf.nfloors; j++)
        msg->stuff.shelf.ndepth[j] = sqlrow[i][2*j]-'0';//提取出ndepth[]
    i++;//完成ndepth[]
    i++;//跳过cleaned
    strcpy(msg->stuff.shelf.building_time, sqlrow[i++]);
    if (i != res_fields)
    {
#if DEBUG_TRACE
        fprintf(stderr, "fetch() error: res_fields didn't match while copying data\n");
#endif
        srvdb_free_result();
        return(FETCH_RESULT_ERR);
    }
    return(FETCH_RESULT_CONT);
}

int srvdb_shelf_count(message_cs_t *msg)
{
    shelf_count_t sc;

    msg->stuff.shelf.code = BREAK_LIMIT_INT;
    //书架总数
    if(!srvdb_shelf_find(msg, &sc.shelves_all)){
#if DEBUG_TRACE
        fprintf(stderr, "srvdb_book_count() error: shelf\n");
#endif
        return(0);
    }
    srvdb_free_result();

    *(shelf_count_t *)&msg->extra_info = sc;
    return(1);
}


//
// 以下是关于账户的MYSQL封装
//

int srvdb_account_verify(message_cs_t *msg)
{
    char es_name[BCRYPT_HASHSIZE*+1];
    char db_hash_pw[BCRYPT_HASHSIZE+1];
    account_type_e db_type;
    char temp_str[512];
    char is[1024];
    int res;

    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "Verify account error: msg user undefined.\n");
#endif
        return(0);
    }

    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->stuff.account.name, strlen(msg->stuff.account.name));

    //从db获取用户类型
    sprintf(is, "SELECT type FROM %s where name='%s'", my_bsdq_accounts, es_name);
    res = mysql_query(&my_connection, is);
    if(!res) {
        if(!get_simple_result(&my_connection, temp_str)){
#if DEBUG_TRACE
            fprintf(stderr, "Verify account get type error: get_simple_result()\n");
#endif
            return(0);
        }
        sscanf(temp_str, "%d", (int *)&db_type);
        if(db_type != msg->stuff.account.type){
            sprintf(msg->error_text, "帐号或密码错误");
            return(0);
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "Verify account get type error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }

    //从db获取hash
    sprintf(is, "SELECT password FROM %s where name='%s'", my_bsdq_accounts, es_name);
    res = mysql_query(&my_connection, is);
    if(!res) {
        if(!get_simple_result(&my_connection, db_hash_pw)){
#if DEBUG_TRACE
            fprintf(stderr, "Verify account get password error: get_simple_result()\n");
#endif
            return(0);
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "Verify account get password error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }

    //验证hash
    res = bcrypt_checkpw(msg->stuff.account.password, db_hash_pw);

    if(res == -1){
#if DEBUG_TRACE
        fprintf(stderr, "Verify account error: checkpw error.\n");
#endif
        return(0);
    }
    //验证成功
    if(res == 0)return(1);

    //验证失败
    sprintf(msg->error_text, "帐号或密码错误");
    return(0);
}

int srvdb_account_register(message_cs_t *msg)
{
    char es_name[BCRYPT_HASHSIZE*2 + 1];
    char es_password[BCRYPT_HASHSIZE*2 + 1];
    char hash_pw[BCRYPT_HASHSIZE];
    char salt[BCRYPT_HASHSIZE];
    char temp_str[512];
    char is[1024];
    int res, user_count;

    if (msg->user[0] == '\0'){
#if DEBUG_TRACE
        fprintf(stderr, "register account error: msg user undefined.\n");
#endif
        return(0);
    }

    //查询用户数
    sprintf(is, "SELECT COUNT(1) FROM %s", my_bsdq_accounts);
    res = mysql_query(&my_connection, is);
    if(!res) {
        if(get_simple_result(&my_connection, temp_str)){
            if(temp_str[0] != '\0'){
                sscanf(temp_str, "%d", &user_count);
                if(user_count >= MAX_USER_NUM){
                    sprintf(msg->error_text, "创建账户失败:用户数达到上限");
                    return(0);
                }
            }
        }else{
#if DEBUG_TRACE
            fprintf(stderr, "Register account error: count users error when calling get_simple_result()\n");
#endif
            return(0);
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "SELECT COUNT(1) count users error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }
         
    //保护字符串中的特殊字符
    mysql_escape_string(es_name, msg->stuff.account.name, strlen(msg->stuff.account.name));

    //用户名已存在?
    sprintf(is, "SELECT count(1) FROM %s where name='%s'", my_bsdq_accounts, es_name);
    res = mysql_query(&my_connection, is);
    if(!res) {
        if(get_simple_result(&my_connection, temp_str)){
            if(temp_str[0] != '\0'){
                sscanf(temp_str, "%d", &user_count);
                if(user_count != 0){
                    sprintf(msg->error_text, "创建账户失败:用户已存在");
                    return(0);
                }
            }
        }else{
#if DEBUG_TRACE
            fprintf(stderr, "Register account error: duplicate checking error when calling get_simple_result()\n");
#endif
            return(0);
        }
    }else{
#if DEBUG_TRACE
        fprintf(stderr, "SELECT COUNT(1) duplicate checking error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
        return(0);
    }

    //产生hash
    res = bcrypt_gensalt(BCRYPT_WORK_FACTOR, salt);
    if(res != 0){
#if DEBUG_TRACE
        fprintf(stderr, "register account error: generate salt error.\n");
#endif
        return(0);
    }

    res = bcrypt_hashpw(msg->stuff.account.password, salt, hash_pw);
    if(res != 0){
#if DEBUG_TRACE
        fprintf(stderr, "register account error: hash password error.\n");
#endif
        return(0);
    }

    //保护字符串中的特殊字符
    mysql_escape_string(es_password, hash_pw, strlen(hash_pw));

    sprintf(is, "INSERT INTO %s(name, password, type) VALUES('%s', '%s', %d)", my_bsdq_accounts, es_name, es_password, msg->stuff.account.type);
    res = mysql_query(&my_connection, is);
    if(!res)return(1);
#if DEBUG_TRACE
        fprintf(stderr, "Create account error %d: %s\n", mysql_errno(&my_connection), mysql_error(&my_connection));
#endif
    return(0);
}
