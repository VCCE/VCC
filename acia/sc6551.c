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
// received a copy of the GNU General Public License along with VCC
// (Virtual Color Computer). If not see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------

#include "acia.h"
#include "sc6551.h"
#include "logger.h"

//------------------------------------------------------------------------
// Handles and buffers for I/O threads
//------------------------------------------------------------------------

// Input buffer 
#define IBUFSIZ 1024
DWORD WINAPI sc6551_input_thread(LPVOID);
HANDLE hInputThread;
HANDLE hStopInput;
char InBuf[IBUFSIZ];
char *InBufPtr = InBuf;
int InBufCnt = 0;

// Output buffer
#define OBUFSIZ 1024
DWORD WINAPI sc6551_output_thread(LPVOID);
HANDLE hOutputThread;
HANDLE hStopOutput;
char OutBuf[OBUFSIZ];
char *OutBufPtr = OutBuf;
int OutBufFree = 0;

int sc6551_initialized = 0;

int BaudRate = 0;  // External Xtal default is ~9600

// Heartbeat counter used to pace recv
int HBtimer=0;

// BaudDelay table is used to set HBtimer
// TODO: Replace wags with good values
int BaudDelay[16] = {16,2200,2000,1600,1200,600,300,150,
	                 128,96,64,48,32,25,16,8};

//------------------------------------------------------------------------
//  Nicely terminate an I/O thread
//------------------------------------------------------------------------
void sc6551_terminate_thread(HANDLE hthread, HANDLE hstop)
{
    if (hthread) {
        // Tell thread to exit
        SetEvent(hstop);
        // If that fails terminate it
        if (WaitForSingleObject(hthread,500) == WAIT_TIMEOUT) {
            TerminateThread(hthread,1);
            WaitForSingleObject(hthread,500);
        }
        CloseHandle(hthread);
        CloseHandle(hstop);
        hthread = NULL;
    }
}

//------------------------------------------------------------------------
//  Initiallize sc6551.  This gets called when DTR is raised.
//------------------------------------------------------------------------
void sc6551_init()
{
    DWORD id;

    if (sc6551_initialized==1) return;

    // Open comunication
    com_open();

    // Create I/O threads
    hInputThread =  CreateThread(NULL,0,sc6551_input_thread, NULL,0,&id);
    hOutputThread = CreateThread(NULL,0,sc6551_output_thread,NULL,0,&id);

    // Create events to terminate them
    hStopInput = CreateEvent(NULL,TRUE,FALSE,NULL);
    hStopOutput = CreateEvent(NULL,TRUE,FALSE,NULL);

    // Clear status register and mark buffers empty
    InBufCnt = 0;
    InBufPtr = InBuf;
    OutBufFree = OBUFSIZ;
    OutBufPtr = OutBuf;
    sc6551_initialized = 1;
}

//------------------------------------------------------------------------
//  Close sc6551. This gets called when DTR is cleared
//------------------------------------------------------------------------
void sc6551_close()
{
    sc6551_terminate_thread(hInputThread, hStopInput);
    sc6551_terminate_thread(hOutputThread, hStopOutput);
    com_close();
    sc6551_initialized = 0;
}

//------------------------------------------------------------------------
// Input thread loads the input buffer
//------------------------------------------------------------------------

DWORD WINAPI sc6551_input_thread(LPVOID param)
{
	// Delay after loading buffer with data.
    int delay=5;

    while(TRUE) {

        if (InBufCnt < 1) {
        // Write only mode: if text buffer an CR+EOF else a null
        if (AciaComMode == COM_MODE_WRITE) {
            if (AciaTextMode) {
                InBuf[0] = '\r';
                InBuf[1] = EOFCHR;
                InBufCnt = 2;
            } else {
                InBuf[0] = 0;
                InBufCnt = 1;
            }
            InBufPtr = InBuf;
            delay = 1000;

	    // Otherwise load buffer with input data
        } else {
            int count = com_read(InBuf,IBUFSIZ);  // blocks
            if (count > 0) {
                delay = 5;
            } else {
                count = 0;
                delay = 1000;
            }
            InBufCnt = count;
            InBufPtr = InBuf;
        }
        }
        if (WaitForSingleObject(hStopInput,delay) != WAIT_TIMEOUT)
                ExitThread(0);
    }
}

//------------------------------------------------------------------------
// Output thread sends the output buffer
//------------------------------------------------------------------------

DWORD WINAPI sc6551_output_thread(LPVOID param)
{
    int delay=50;

    OutBufPtr = OutBuf;
    OutBufFree = OBUFSIZ;
    StatReg |= StatTxE;

	while(TRUE) {
        // If Read Mode writes go to the bit bucket
        if (AciaComMode == COM_MODE_READ) {
            OutBufPtr = OutBuf;
            OutBufFree = OBUFSIZ;
            delay = 1000;
        } else {
			// Clear transmit ready
            StatReg &= ~StatTxE;
			// Flush output buffer
            char * ptr = OutBuf;
            while (TRUE) {
                int towrite = OutBufPtr - ptr;
                if (towrite < 1) break;
                int written = com_write(ptr,towrite);
                if (written < 1) {
                    break;
                } else {
                    ptr = ptr + written;
                }
            }
            OutBufPtr = OutBuf;
            OutBufFree = OBUFSIZ;
			// Set transmit ready 
            StatReg |= StatTxE;
            delay = 50;
		}
        if (WaitForSingleObject(hStopOutput, delay) != WAIT_TIMEOUT)
            if (OutBufFree == OBUFSIZ) ExitThread(0);
    }
}

//------------------------------------------------------------------------
// Use Heartbeat (HSYNC) to control receive rate 
//------------------------------------------------------------------------
void sc6551_heartbeat()
{
	// Countdown to process next byte
	if (HBtimer-- > 0) return;
    HBtimer = BaudDelay[BaudRate];

	// Set RxF if there is data in the input buffer
    if (InBufCnt > 0) { 
        StatReg |= StatRxF;
		if (!(CmdReg &CmdRxI)) {
            StatReg |= StatIRQ;
	        AssertInt(IRQ,0);
		}
    }
    return;
}

//------------------------------------------------------------------------
// Port Read
//------------------------------------------------------------------------
unsigned char sc6551_read(unsigned char port)
{
    unsigned char data = 0;
    switch (port) {

        // Read input data
        case 0x68:
            data = *InBufPtr;
            if (InBufCnt > 0) {
                *InBufPtr++;
                InBufCnt--;
            }
            StatReg &= ~StatRxF;
            break;

        // Read status register
        case 0x69:
            if (OutBufFree < 16) StatReg &= ~StatTxE;
			data = StatReg;
            StatReg &= ~StatIRQ;
            break;

        // Read command register
        case 0x6A:
            data = CmdReg;
            break;

        // Read control register
        case 0x6B:
            data = CtlReg;
            break;

        default:
            data = 0;
    }
    return data;
}

//------------------------------------------------------------------------
// Port Write
//------------------------------------------------------------------------
void sc6551_write(unsigned char data,unsigned short port)
{
    switch (port) {
        // Data
        case 0x68:
            if (OutBufFree-- > 0) *OutBufPtr++ = data;
            //Software flow control here? if (data == XOFCHR ) ... 
            break;
        // Clear status
        case 0x69:
            StatReg = 0;
            break;
        // Write Command register
        case 0x6A:
            CmdReg = data;
            if (CmdReg & CmdDTR) {
                if (sc6551_initialized != 1) sc6551_init();
            } else {
                if (sc6551_initialized == 1) sc6551_close();
            }
			// Cmd write causes delay in real hardware
			// Simulate the delay with a long HB timer 
			// (os9 sc6551 driver needs this for flow control)
            HBtimer=1024;  
			break;
        // Write Control register
        case 0x6B:
            CtlReg = data;
            BaudRate = CtlReg & CtlBaud;
			break;
    }
}
