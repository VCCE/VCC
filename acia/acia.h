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

#define DIRECTINPUT_VERSION 0x0800
#define MAX_LOADSTRING 200

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <conio.h>
#include <dinput.h>
#include "resource.h"

// Dynamic menu control
#define HEAD 0
#define SLAVE 1
#define STANDALONE 2

// Text mode EOF character
#define EOFCHR 0x1B

// Communications type and mode enumerations
enum com_type {
    COM_CONSOLE,
    COM_FILE,
    COM_TCPIP,
    COM_WINCOM
};
enum com_mode {
    COM_MODE_DUPLEX,
    COM_MODE_READ,
    COM_MODE_WRITE,
};

// Handles for I/O threads
HANDLE hInputThread;
HANDLE hOutputThread;
HANDLE hStopInput;
HANDLE hStopOutput;

// Config globals
int  AciaComType;            // Console,file,tcpip,wincom
int  AciaComMode;            // Duplex,read,write
int  AciaTextMode;           // CR and EOF translations 0=none 1=text
int  AciaLineInput;          // Console line mode 0=Normal 1=Linemode
int  AciaComPort;            // COM port 0-99
int  AciaTcpPort;            // TCP port 1024-65536
char AciaTcpHost[MAX_PATH];  // Tcpip hostname 
char AciaFilePath[MAX_PATH]; // Path for file I/O

// Status for Vcc status line
char AciaStat[32];

// Comunications hooks
int  com_open();
void com_close();
int  com_write(char*,int);
int  com_read(char*,int);

// Console
int  console_open();
void console_close();
int  console_read(char*,int);
int  console_write(char*,int);

// File
int  file_open();
void file_close();
int  file_read(char*,int);
int  file_write(char*,int);

// Tcpip
int  tcpip_open();
void tcpip_close();
int  tcpip_read(char*,int);
int  tcpip_write( char*,int);

// WinCom 
int  wincom_open();
void wincom_close();
int  wincom_read(char*,int);
int  wincom_write(char*,int);

#endif
