#include <stdio.h>
#include <openssl/aes.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_PATH "aes/.aes_key.config"

unsigned char key[AES_BLOCK_SIZE];        // AES_BLOCK_SIZE = 16
int aes_has_init = 0;

int AES_init(void)
{
    int i = 0;
    FILE *fp;
    int tkey[AES_BLOCK_SIZE];

    if(aes_has_init)return(1);

    // Generate AES 128-bit key
    fp = fopen(CONFIG_PATH, "r+");
    if(fp == NULL){
        perror("fopen()");
        return(0);
    }
    fscanf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",\
            &tkey[0],&tkey[1],&tkey[2],&tkey[3],&tkey[4],&tkey[5],&tkey[6],&tkey[7],\
            &tkey[8],&tkey[9],&tkey[10],&tkey[11],&tkey[12],&tkey[13],&tkey[14],&tkey[15]);
    for(i = 0; i < 16; i ++)
        key[i] = tkey[i];

    aes_has_init = 1;
    fclose(fp);
    return(1);
}

int encrypt(unsigned char *input_string, int len, unsigned char *encrypt_string)
{
    AES_KEY aes;
    unsigned char iv[AES_BLOCK_SIZE];        // init vector
    unsigned int i;

    if(!aes_has_init){
#ifdef DEBUG_TRACE
        fprintf(stderr, "aes key has not been initialized.\n");
#endif
        return(-1);
    }

    // set the encryption length
    if (len % AES_BLOCK_SIZE != 0)
        len = (len / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;

    // Set encryption key
    for (i=0; i<AES_BLOCK_SIZE; ++i)
        iv[i] = 0;
    if (AES_set_encrypt_key(key, 128, &aes) < 0) {
        fprintf(stderr, "Unable to set encryption key in AES\n");
        return(-1);
    }

    // encrypt (iv will change)
    AES_cbc_encrypt(input_string, encrypt_string, len, &aes, iv, AES_ENCRYPT);

    return len;
}

int decrypt(unsigned char *encrypt_string, unsigned char *decrypt_string, int len)
{
    unsigned char iv[AES_BLOCK_SIZE];        // init vector
    AES_KEY aes;
    int i;

    if(!aes_has_init){
#ifdef DEBUG_TRACE
        fprintf(stderr, "aes key has not been initialized.\n");
#endif
        return(-1);
    }

    // Set decryption key
    for (i=0; i<AES_BLOCK_SIZE; ++i)
        iv[i] = 0;
    if (AES_set_decrypt_key(key, 128, &aes) < 0) {
        fprintf(stderr, "Unable to set decryption key in AES\n");
        return(-1);
    }

    // decrypt
    AES_cbc_encrypt(encrypt_string, decrypt_string, len, &aes, iv, AES_DECRYPT);

    return(1);
}
