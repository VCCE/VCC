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

// sc6551 calls
void sc6551_init();
void sc6551_close();
void sc6551_ping();
unsigned char sc6551_read(unsigned char data);
void sc6551_write(unsigned char data, unsigned short port);
void (*AssertInt)(unsigned char,unsigned char);

// sc6551 status
int sc6551_initialized; // initilization state

// Status Register
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
#define StatPar  0x01
#define StatFrm  0x02
#define StatOvr  0x04
#define StatRxF  0x08
#define StatTxE  0x10
#define StatDCD  0x20
#define StatDSR  0x40
#define StatIRQ  0x80

// Command register
unsigned char CmdReg;
// Command register bits.
// b0   DTR Enable receive and interupts
// b1   RxI Receiver IRQ control by StatRxF
// b2-3 TIRB Transmit IRQ control
//      00 trans disabled,01 enabled,10 disabled,11 break
// b4   Echo Set=Activated
// b5-7 Par  Parity control
//      xx0 none, 001 Odd, 011 Even, 101 mark, 111 space

#define CmdDTR  0x01
#define CmdRxI  0x02
#define CmdTIRB 0x0C
#define CmdEcho 0x10
#define CmdPAR  0xE0

#define TIRB_Off  0x00
#define TIRB_On   0x04
#define TIRB_RTS  0x08
#define TIRB_Brk  0x0C

// Control Register
unsigned char CtlReg;
// b0-3 Baud rate
//		{ X,60,75,110,135,150,300,600,1200,
//		  1800,2400,3600,4800,7200,9600,19200 }
// b4   Rcv Clock source 0=X 1=intenal
// b5-6 Data len 00=8 01=7 10=6 11=5
// b7   Stop bits 0=1, 1-2

