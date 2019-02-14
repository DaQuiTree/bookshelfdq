#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gdbm-ndbm.h>

#include "clidb.h"
#include "clisrv.h"

#define CACHE_DIR "bsdq_cache"
#define BOOK_FILE_BASE "bsdq_cache/book_data"
#define BOOK_FILE_DIR "bsdq_cache/book_data.dir"
#define BOOK_FILE_PAG "bsdq_cache/book_data.pag"
#define SHELF_FILE_BASE "bsdq_cache/shelf_data"
#define SHELF_FILE_DIR "bsdq_cache/shelf_data.dir"
#define SHELF_FILE_PAG "bsdq_cache/shelf_data.pag"

//当前登录用户
static char current_user[USER_NAME_LEN+1];

//dbm指针
static DBM *shelf_dbm_ptr = NULL;
static DBM *book_dbm_ptr = NULL;

//记录存入数据库的书架编号
static int shelf_record_init = 1;
static int shelf_record[MAX_SHELF_NUM] = {0};
#define SHELF_RECORD_UNSET(x) (shelf_record[x-1] = 0)
#define SHELF_RECORD_SET(x) (shelf_record[x-1] = 1)
#define SHELF_RECORD_SYNC(x) (shelf_record[x-1] = 2)

#define SHELF_INSERT_MODE 0
#define SHELF_SYNC_MODE 1

//书籍相关变量
static int book_dbm_pos = 0;
static int book_search_pos = 0;
static int book_search_step = 10;

static int shelf_record_save(void);
static int shelf_record_obtain(void);
static int clidb_shelf_real_insert(shelf_entry_t *user_shelf, int mode);

int clidb_init(char* login_user)
{
    int open_mode = O_CREAT | O_RDWR;
    int result;

    strcpy(current_user, login_user);

    //创建存放dbm文件的文件夹
    if(access(CACHE_DIR, F_OK) != 0){
        result = mkdir(CACHE_DIR, 0755);
        if(result != 0){
#if DEBUG_TRACE
            perror("mkdir");
#endif
            return (0);
        }
    }

    //打开dbm文件
    if (shelf_dbm_ptr) dbm_close(shelf_dbm_ptr);
    if (book_dbm_ptr) dbm_close(book_dbm_ptr);
    unlink(BOOK_FILE_DIR);//每次登录都清空book相关dbm文件
    unlink(BOOK_FILE_PAG);
    shelf_dbm_ptr = dbm_open(SHELF_FILE_BASE, open_mode, 0644);
    book_dbm_ptr = dbm_open(BOOK_FILE_BASE, open_mode, 0644);
    if (!shelf_dbm_ptr || !book_dbm_ptr){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_init() error: dbm_open() return null.\n");
#endif
        return (0);
    }

    //获取存入数据库的书架编号
    (void)shelf_record_obtain();
    return (1);
}

int clidb_shelf_insert(shelf_entry_t *user_shelf)
{
   return(clidb_shelf_real_insert(user_shelf, SHELF_INSERT_MODE));
}

int clidb_shelf_synchronize(shelf_entry_t *user_shelf)
{
    return(clidb_shelf_real_insert(user_shelf, SHELF_SYNC_MODE));
}

int clidb_shelf_delete(int shelfno)
{
    datum local_key_datum;
    int result = 0;

    char key_to_del[USER_NAME_LEN + 16 + 1];

    if(!shelf_dbm_ptr){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_delete() error: shelf dbm has not been initialized.\n");
#endif
        return 0;
    }
    memset(key_to_del, '\0', sizeof(key_to_del));
    sprintf(key_to_del, "%s_shelf_%d", current_user, shelfno);
    local_key_datum.dptr = (void *)key_to_del;
    local_key_datum.dsize = sizeof(key_to_del);
    
    result = dbm_delete(shelf_dbm_ptr, local_key_datum);
    if(result != 0){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_delete() error: delete shelf info failed.\n");
#endif
        return(0);
    } 

    //更新记录
    SHELF_RECORD_UNSET(shelfno);
    if(!shelf_record_save()){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_delete() error: shelf_record_save() failed.\n");
#endif
        return(0);
    }

    return(1);
}

int clidb_shelf_get(int shelfno, shelf_entry_t *user_shelf)
{
    char key_to_get[USER_NAME_LEN + 16 + 1];
    datum local_key_datum;
    datum local_data_datum;

    if(!shelf_dbm_ptr){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_insert() error: shelf dbm has not been initialized.\n");
#endif
        return(0);
    }

    memset(key_to_get, '\0', sizeof(key_to_get));
    sprintf(key_to_get, "%s_shelf_%d", current_user, shelfno);
    local_key_datum.dptr = (void *)key_to_get;
    local_key_datum.dsize = sizeof(key_to_get);

    local_data_datum = dbm_fetch(shelf_dbm_ptr, local_key_datum); 
    if(local_data_datum.dptr){
        memcpy((void *)user_shelf, local_data_datum.dptr, local_data_datum.dsize);
        return(1);
    }

#if DEBUG_TRACE
    fprintf(stderr, "clidb_shelf_get(): did not get shelf record info.\n");
#endif

    return 0;
}

int clidb_shelf_exists(int shelfno)
{
    if (shelfno > MAX_SHELF_NUM)return(0);
    if (shelf_record[shelfno-1] > 0)return(1);
    return(0);
}

int clidb_shelf_not_syncs(int shelfno){
    if (shelfno > MAX_SHELF_NUM)return(0);
    if (shelf_record[shelfno-1] == 1)return(1);
    return(0);
}

static int clidb_shelf_real_insert(shelf_entry_t *user_shelf, int mode)
{
    datum local_key_datum;
    datum local_data_datum;
    int result = 0;

    char key_to_add[USER_NAME_LEN + 16 + 1];//书架编号字符最多占两个字节

    if(!shelf_dbm_ptr){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_insert() error: shelf dbm has not been initialized.\n");
#endif
        return 0;
    }
    memset(key_to_add, '\0', sizeof(key_to_add));
    sprintf(key_to_add, "%s_shelf_%d", current_user, user_shelf->code);
    local_key_datum.dptr = (void *)key_to_add;
    local_key_datum.dsize = sizeof(key_to_add);
    local_data_datum.dptr = (void *)user_shelf;
    local_data_datum.dsize = sizeof(shelf_entry_t);
    result = dbm_store(shelf_dbm_ptr, local_key_datum, local_data_datum, DBM_REPLACE);
    if(result != 0){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_insert() error: store shelf info failed.\n");
#endif
        return(0);
    }
    
    //更新记录
    if(mode == SHELF_SYNC_MODE){
        SHELF_RECORD_SYNC(user_shelf->code);
    }else if(mode == SHELF_INSERT_MODE){
        SHELF_RECORD_SET(user_shelf->code);
    }

    if(!shelf_record_save()){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_insert() error: shelf_record_save() failed.\n");
#endif
        return(0);
    }

    return(1);
}

static int shelf_record_obtain(void)
{
    char key_to_get[USER_NAME_LEN + 16 + 1];
    datum local_key_datum;
    datum local_data_datum;

    if(!shelf_dbm_ptr){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_insert() error: shelf dbm has not been initialized.\n");
#endif
        return(0);
    }

    memset(key_to_get, '\0', sizeof(key_to_get));
    sprintf(key_to_get, "%s_shelf_record", current_user);
    local_key_datum.dptr = (void *)key_to_get;
    local_key_datum.dsize = sizeof(key_to_get);

    local_data_datum = dbm_fetch(shelf_dbm_ptr, local_key_datum); 
    if(local_data_datum.dptr){
        memcpy((void *)shelf_record, local_data_datum.dptr, local_data_datum.dsize);
#if DEBUG_TRACE
        int i;
        for(i = 0; i < MAX_SHELF_NUM; i++){
            fprintf(stderr, "%d ",shelf_record[i]);
        }
        printf("\n");
#endif
        return(1);
    }

#if DEBUG_TRACE
        fprintf(stderr, "shelf_record_obtain(): did not obtain shelf record info.\n");
#endif
        
    shelf_record_init = 1;//未获取到shelf_record,需要初始化
    return(0);
}

static int shelf_record_save(void)
{
    char key_to_add[USER_NAME_LEN + 16 + 1];
    datum local_key_datum;
    datum local_data_datum;
    int result;

    if(!shelf_dbm_ptr){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_insert() error: shelf dbm has not been initialized.\n");
#endif
        return(0);
    }
    memset(key_to_add, '\0', sizeof(key_to_add));
    sprintf(key_to_add, "%s_shelf_record", current_user);
    local_key_datum.dptr = (void *)key_to_add;
    local_key_datum.dsize = sizeof(key_to_add);
    local_data_datum.dptr = (void *)shelf_record;
    local_data_datum.dsize = sizeof(shelf_record);
    result = dbm_store(shelf_dbm_ptr, local_key_datum, local_data_datum, DBM_REPLACE);
    if(result != 0){
#if DEBUG_TRACE
        fprintf(stderr, "shelf_record_save() error: store shelf record failed.\n");
#endif
        return(0);
    }
    return(1);
}

int clidb_shelf_realno(int pos)
{
    int realno = 0;

    while(pos--){
        while(!clidb_shelf_exists(++realno));
    }

    return(realno);
}

int clidb_shelf_record_sort(void) 
{
    int i;

    for(i = 0; i < MAX_SHELF_NUM; i++)
        if(shelf_record[i] == 2)shelf_record[i] = 1;//2:sync
    if(!shelf_record_save()){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_shelf_delete() error: shelf_record_save() failed.\n");
#endif
        return(0);
    }
#if DEBUG_TRACE
        for(i = 0; i < MAX_SHELF_NUM; i++){
            fprintf(stderr, "%d ",shelf_record[i]);
        }
        printf("\n");
#endif
    return(1);
}

void clidb_book_reset(void)
{
    book_dbm_pos = 0;
    book_search_pos = 0;
}


int clidb_book_insert(book_entry_t *user_book)
{
    datum local_key_datum;
    datum local_data_datum;
    int result = 0;

    char key_to_add[16];//书架编号字符最多占两个字节

    if(!book_dbm_ptr){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_book_insert() error: book dbm has not been initialized.\n");
#endif
        return 0;
    }

    memset(key_to_add, '\0', sizeof(key_to_add));
    sprintf(key_to_add, "book_%d", book_dbm_pos++);
    local_key_datum.dptr = (void *)key_to_add;
    local_key_datum.dsize = sizeof(key_to_add);
    local_data_datum.dptr = (void *)user_book;
    local_data_datum.dsize = sizeof(book_entry_t);
    result = dbm_store(book_dbm_ptr, local_key_datum, local_data_datum, DBM_REPLACE);
    if(result != 0){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_book_insert() error: store shelf info failed.\n");
#endif
        return(0);
    }

    return(1);
}

void clidb_book_search_reset(void)
{
    book_search_pos = 0;
}

void clidb_book_search_step(int step)
{
    book_search_step = step;
}

int clidb_book_backward_mode(void)
{
    int temp_pos = 0;

    temp_pos = (book_search_pos-1)/book_search_step*book_search_step - book_search_step;
    if(temp_pos < 0)return(0); 
    book_search_pos = temp_pos;
    return(1);
}   

int clidb_book_get(book_entry_t *user_book)
{
    char key_to_get[16];
    datum local_key_datum;
    datum local_data_datum;

    if(!book_dbm_ptr){
#if DEBUG_TRACE
        fprintf(stderr, "clidb_book_insert() error: book dbm has not been initialized.\n");
#endif
        return(0);
    }

    if(book_search_pos > book_dbm_pos-1)return(-1);
    memset(key_to_get, '\0', sizeof(key_to_get));
    sprintf(key_to_get, "book_%d", book_search_pos++);
    local_key_datum.dptr = (void *)key_to_get;
    local_key_datum.dsize = sizeof(key_to_get);

    local_data_datum = dbm_fetch(book_dbm_ptr, local_key_datum); 
    if(local_data_datum.dptr){
        memcpy((void *)user_book, local_data_datum.dptr, local_data_datum.dsize);
        return(1);
    }

#if DEBUG_TRACE
    fprintf(stderr, "clidb_book_forward_get(): did not get book info.\n");
#endif

    return 0;
}

