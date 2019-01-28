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
    char abc[17] = "我爱小花";
    printf("%s %d\n", abc, strlen(abc));
    c.b = 10;
    printf("%d\n",c.b);
    c.a = 'c';
    printf("%c\n", c.a);
}
