#ifndef __BECKER_H__
#define __BECKER_H__

// CAUTION these are different from the standalone becker defines
void becker_enable(bool);                        // enable or disable
int becker_sethost(char *, char *);              // server ip address, port
unsigned char becker_read(unsigned short);       // coco port
void becker_write(unsigned char,unsigned short); // value, coco port
void becker_status(char *);                      // becker status for status line
#endif
