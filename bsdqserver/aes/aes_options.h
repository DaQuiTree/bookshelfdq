#ifndef _AES_OPTION_H_
#define _AES_OPTION_H_

int AES_init(void);
int encrypt(unsigned char *input_string, int len, unsigned char *encrypt_string);
int decrypt(unsigned char *encrypt_string, unsigned char *decrypt_string, int len);

#endif
