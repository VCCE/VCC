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

// sc6551 calls
void sc6551_init();
void sc6551_close();
void sc6551_ping();
unsigned char sc6551_read(unsigned char data);
void sc6551_write(unsigned char data, unsigned short port);
void (*AssertInt)(unsigned char,unsigned char);

// sc6551 status 
int sc6551_initialized; // initilization state

unsigned char StatReg;
// Status register bits.
// b0 Par Rx parity error
// b1 Frm Rx framing error
// B2 Ovr Rx data over run
// b3 RxF Rx Data register full
// b4 TxE Tx Data register empty
// b5 DCD Data carrier detect
// b6 DSR Data set ready
// b7 IRQ Interrupt read status
#define StatOvr  0x04
#define StatRxF  0x08
#define StatTxE  0x10
#define StatDCD  0x20
#define StatDSR  0x40
#define StatIRQ  0x80

unsigned char CmdReg;
// Command register bits.
// b0   DTR Enable receive and interupts
// b1   RxI Receiver IRQ enable.
// b2-3 TIRB Transmit IRQ control
// B4   Echo Set=Activated
// B5-7 Par  Parity control
#define CmdDTR  0x01
#define CmdRxI  0x02
#define CmdEcho 0x10

unsigned char CtlReg;
// b0-3 Baud rate 
//		{ X,60,75,110,135,150,300,600,1200,
//		  1800,2400,3600,4800,7200,9600,19200 }
// b4   Rcv Clock source 0=X 1=intenal
// b5-6 Data len 00=8 01=7 10=6 11=5
// b7   Stop bits 0=1, 1-2

// Console I/O
void console_open();
void console_close();
int  console_read(char* buf,int siz);
int  console_write(char* buf,int siz);
int ConsoleLineInput;  // Console mode Normal: 0; Line mode: 1

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
