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
int  com_write(char*,int);
int  com_read(char*,int);

// Communications media
enum comtype {
    COM_CONSOLE,
    COM_FILE,
    COM_TCPIP,
    COM_WINCOM
};
int AciaComType;


// Communications mode, duplex, read only, or write only
enum com_mode {
    COM_MODE_DUPLEX,
    COM_MODE_READ,
    COM_MODE_WRITE,
};
int AciaComMode;

// Line ending and EOF translations 0=none 1=text
int AciaTextMode;

// Character used to indicate end of file when text mode is used 
#define EOFCHR 0x1B
// Flow control character when text mode is used
#define XOFCHR 0x0D

// Console 
int  console_open();
void console_close();
int  console_read(char* buf,int siz);
int  console_write(char* buf,int siz);
// Console mode toggle Normal: 0; Line mode: 1
int  ConsoleLineInput;  

// File 
int  file_open();
void file_close();
int  file_read(char* buf,int siz);
int  file_write(char* buf,int siz);
// Path for file I/O
char AciaFilePath[MAX_PATH];

// Status for Vcc status line
char AciaStat[32];

// Port numbers: COM port 1-10; TCP port 1024-65536
int AciaComPort;
int AciaTcpPort;

//menu control
#define HEAD 0
#define SLAVE 1
#define STANDALONE 2

#endif
