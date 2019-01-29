#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union{
    char a;
    int b;
}love_un;

int main()
{
    love_un c;
    char str[16]="";
    int a = -32760;

    c.b = 10;
    printf("%d\n",c.b);
    c.a = 'c';
    printf("%c\n", c.a);
    printf("%d %d\n", a, -a);
}
