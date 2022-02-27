#ifndef __ACIA_H_
#define __ACIA_H_

#define IRQ 1
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <dinput.h>
#include "resource.h"

void sc6551_init();
void sc6551_close();

unsigned char sc6551_read(unsigned char data);
void sc6551_write(unsigned char data, unsigned short port);

void console_open();
void console_close();
int  console_read(char* buf,int siz);
int  console_write(char* buf,int siz);

void (*AssertInt)(unsigned char,unsigned char);

// sc6551 state
int	sc6551_initialized;

// Communications type: console 0; TCP port 1; COM port 2
int AciaComType; 

// Port numbers: COM port 1-10; TCP port 1024-65536 
int AciaComPort;
int AciaTcpPort;

// Console input mode: Normal: 0; Line mode: 1
int ConsoleLineInput;

//menu
//#define HEAD 0
//#define SLAVE 1
#define STANDALONE 2

#endif
