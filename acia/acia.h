//------------------------------------------------------------------
// Copyright E J Jaquay 2022
//
// This file is part of VCC (Virtual Color Computer).
//
// VCC (Virtual Color Computer) is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.  You should have
// received a copy of the GNU General Public License  along with VCC 
// (Virtual Color Computer). If not see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------

#ifndef __ACIA_H_
#define __ACIA_H_

#define IRQ 1
#define DIRECTINPUT_VERSION 0x0800
#define MAX_LOADSTRING 200

#include <windows.h>
#include <windowsx.h> 
#include <stdio.h>
#include <conio.h>
#include <dinput.h>
#include "resource.h"

// Comunications hooks
int  com_open();
void com_close();
void com_set(int,int);
int  com_write(char*,int);
int  com_read(char*,int);

// Communications types
enum comtype {
    COM_CONSOLE,  //0
    COM_FILE,     //1
    COM_TCPIP,    //2
    COM_WINCOM    //3
};

// File mode
enum file_mode {
    FILE_NONE,   //0
    FILE_READ,   //1
    FILE_WRITE,  //2
};
int AciaFileMode;

int AciaTextMode; // 0=binary I/O, 1=text I/O

// Items for com_set
enum set_item {
    LOCAL_ECHO
};

// Console I/O
int  console_open();
void console_close();
void console_set(int,int);
int  console_read(char* buf,int siz);
int  console_write(char* buf,int siz);

int  ConsoleLineInput;  // Console mode Normal: 0; Line mode: 1

// File I/O
int  file_open();
void file_close();
int  file_read(char* buf,int siz);
int  file_write(char* buf,int siz);

char AciaFilePath[MAX_PATH];

// cmd I/O
int  wincmd_open();
void wincmd_close();
void wincmd_set(int,int);
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
