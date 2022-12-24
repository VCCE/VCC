/*
Copyright E J Jaquay 2022
This file is part of VCC (Virtual Color Computer).
    VCC (Virtual Color Computer) is free software: you can redistribute it
    and/or modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ACIA_H_
#define __ACIA_H_

#define IRQ 1
#define DIRECTINPUT_VERSION 0x0800
#define MAX_LOADSTRING 200

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <dinput.h>
#include "resource.h"

// Comunications hooks
void com_open();
void com_close();
int  com_write(char*,int);
int  com_read(char*,int);

// Console I/O
void console_open();
void console_close();
int  console_read(char* buf,int siz);
int  console_write(char* buf,int siz);
int  ConsoleLineInput;  // Console mode Normal: 0; Line mode: 1

// cmd I/O
void wincmd_open();
void wincmd_close();
int  wincmd_read(char* buf,int siz);
int  wincmd_write(char* buf,int siz);

// acia status 
char AciaStat[32];

// Communications type: console 0; TCP port 1; COM port 2
int AciaComType;

// Port numbers: COM port 1-10; TCP port 1024-65536
int AciaComPort;
int AciaTcpPort;

//menu control
#define HEAD 0
#define SLAVE 1
#define STANDALONE 2

#endif
