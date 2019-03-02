#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

static char *limit_str_width(const char *ostr, char *newstr, int limit);

int main()
{
    int a;
/*    int cpy;*/
    /*char str[128];*/

    /*char *a = "0123456789012345678901234567890123456789";*/
    /*char *b = "张振奎爱小麦呀真的哦我从来";*/
    /*char *c = "abc我爱你cdefg我的1111111111我是我贺我是地";*/
    /*char *d = "；：‘’‘’‘’‘’‘’‘’‘’！！！！！！？～。，《》";*/

    /*printf("%s\n", limit_str_width(a, str, 23));*/
    /*printf("%s\n", limit_str_width(b, str, 23));*/
    /*printf("%s\n", limit_str_width(c, str, 23));*/
    /*printf("%s\n", limit_str_width(d, str, 23));*/
    while(1){
        a = getchar();
        printf("%d\n", a);
        if(a == '\n')break;
    }
}

static char *limit_str_width(const char *oldstr, char *newstr, int limit)
{
    const char *sptr;
    int width = 0;
    int flag_stop = 0;

    sptr = oldstr;
    while(*sptr != '\0' && flag_stop == 0)
    {
        if((*sptr & 0x80) == 0){
            if(++width > limit){
                flag_stop = 1;
                break;
            }
            sptr++;
        }else{
            switch(*sptr & 0xF0)
            {
                case 0xC0:
                    width += 2;
                    if(width > limit){
                        flag_stop = 1;
                        break;
                    }
                    sptr += 2;
                    break;
                case 0xE0:
                    width += 2;
                    if(width > limit){
                        flag_stop = 1;
                        break;
                    }
                    sptr += 3;
                    break;
                case 0xF0:
                    width += 2;
                    if(width > limit){
                        flag_stop = 1;
                        break;
                    }
                    sptr += 4;
                    break;
                default: break;
            }
        }   
    }

    strncpy(newstr, oldstr, sptr-oldstr);
    newstr[sptr-oldstr] = '\0';
    if(flag_stop)strcat(newstr, "...");
    return(newstr);
}
