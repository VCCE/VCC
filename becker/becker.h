#ifndef __BECKER_H__
#define __BECKER_H__

// functions
void MemWrite(unsigned char,unsigned short );
unsigned char MemRead(unsigned short );
void BuildDynaMenu(void);

// settings.. should these be settings in the gui?
#define BUFFER_SIZE 512
#define TCP_RETRY_DELAY 500
#define STATS_PERIOD_MS 100

//Misc
#define QUERY 255

//Common CPU defs
#define IRQ		1
#define FIRQ	2
#define NMI		3

#endif // __BECKER_H__

