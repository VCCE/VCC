#ifndef __BECKER_H__
#define __BECKER_H__

void becker_enable(bool);                        // enable or disable
int becker_sethost(const char *, const char *);  // server ip address, port
unsigned char becker_read(unsigned short);       // coco port
void becker_write(unsigned char,unsigned short); // value, coco port
void becker_status(char *);                      // becker status for status line
#endif
