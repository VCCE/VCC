#ifndef __BECKER_H__
#define __BECKER_H__

#ifndef __cplusplus
#define bool int
#define true 1
#define false 0
#endif

// functions
void MemWrite(unsigned char,unsigned short );
unsigned char MemRead(unsigned short );

extern const unsigned char Rom[8192];

// settings.. should these be settings in the gui?
#define BUFFER_SIZE 512
#define TCP_RETRY_DELAY 500
#define STATS_PERIOD_MS 100

#endif
