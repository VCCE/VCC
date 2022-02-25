

#define IRQ 1
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <dinput.h>
#include "resource.h"

unsigned char sc6551_read(unsigned char);
void sc6551_write(unsigned char,unsigned short);
void sc6551_open();
void sc6551_close();

void acia_open_com();
void acia_close_com();
int  acia_write_com(char*,int);
int  acia_read_com(char*,int);

void console_open();
void console_close();
int  console_read(char*,int);
int  console_write(char*,int);

void (*AssertInt)(unsigned char,unsigned char);

// Console input mode.  Set to enable line mode
int ConsoleLineInput;

int	sc6551_initialized;


