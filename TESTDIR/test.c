#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

typedef union{
    char a;
    int b;
}love_un;

int main()
{
    love_un c;
    char str[16]="";
    int a = -32760;
    int result; 

    c.b = 10;
    printf("%d\n",c.b);
    c.a = 'c';
    //printf("%c\n", c.a);
    //printf("%d %d\n", a, -a);
    //printf("%d %d\n",str, &str);
    if(!access("bsdq_cache", F_OK)){
        printf("exist!\n");
    }else{
        mkdir("bsdq_cache", 0777);
        printf("%d\n", result);
    }
}
