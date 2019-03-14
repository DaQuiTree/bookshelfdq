#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define CONFIG_PATH "./.aes_key.config"

int main(void)
{
    int i;
    unsigned char randch;
    time_t the_time;
    FILE* fp;
    char key[128];
    char buf[8];

    fp = fopen(CONFIG_PATH, "w+");
    if(fp == NULL){
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    (void)time(&the_time);
    srand((unsigned int)the_time);

    for(i = 0; i < 16; i++){
        memset(buf, '\0', 8);
        randch = rand()%255;
        sprintf(buf, "%d,", randch);
        strcat(key, buf);
    }

    key[strlen(key)-1] = '\0';

    fprintf(fp,"%s",key);
    fclose(fp);

    fprintf(stdout, "generate AES key successfully!\n");
    return(EXIT_SUCCESS);
}
