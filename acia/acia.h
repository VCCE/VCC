

#define IRQ 1
#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <dinput.h>
#include "resource.h"

unsigned char sc6551_read(unsigned char);
void sc6551_write(unsigned char,unsigned short);
void acia_open_com();
void acia_write_com(unsigned char);

unsigned char acia_read_com();
void acia_close_com();

void console_open();
void console_close();
unsigned char console_read();
void console_write(unsigned char);

void (*AssertInt)(unsigned char,unsigned char);

