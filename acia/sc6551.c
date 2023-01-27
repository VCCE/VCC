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

// sc6551 registers
unsigned char StatReg;
unsigned char CmdReg;
unsigned char CtlReg;

// Input buffer
#define IBUFSIZ 1024
DWORD WINAPI sc6551_input_thread(LPVOID);
HANDLE hInputThread;
HANDLE hStopInput;
char InBuf[IBUFSIZ];
char *InRptr = InBuf;  // Input buffer read pointer
char *InWptr = InBuf;  // Input buffer write pointer

// Output buffer
#define OBUFSIZ 1024
DWORD WINAPI sc6551_output_thread(LPVOID);
HANDLE hOutputThread;
HANDLE hStopOutput;
char OutBuf[OBUFSIZ];
char *OutWptr = OutBuf;

int sc6551_initialized = 0;

int BaudRate = 0;  // External Xtal default is ~9600

// Heartbeat counter used to pace recv
int HBtimer = 0;

// Thread keep alive timer
int LastTickCount=0;

// BaudDelay table is used to set HBtimer
// TODO: Replace wags with good values
int BaudDelay[16] = {0,2200,2000,1600,1200,600,300,150,
                     120,90,60,40,30,20,10,5};

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
    InRptr = InBuf; InWptr = InBuf;
    OutWptr = OutBuf;
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
// Input thread
//------------------------------------------------------------------------
DWORD WINAPI sc6551_input_thread(LPVOID param)
{
    int delay;
    int avail = IBUFSIZ;

    while(TRUE) {
        if (avail > IBUFSIZ/4) {      // Avoid short reads
            if (AciaComMode == COM_MODE_WRITE) {
                *InBuf = '\0';
                InWptr = InBuf+1;
                delay = 2000;
            } else {
                int cnt = com_read((char *)InWptr,avail);
//                PrintLogF("R%d ",cnt);
                if (cnt > 0) {
                    InWptr += cnt;
                    avail -= cnt;
                }
                delay = 0;
            }
        } else if (InRptr >= InWptr) { // If coco caught up
            InWptr = InBuf;            //    Reset buffer
            InRptr = InBuf;
            avail = IBUFSIZ;
            delay = 0;
        } else {                       // Else nap until coco is done.
            delay = 20;
        }

        if (WaitForSingleObject(hStopInput,delay) != WAIT_TIMEOUT) {
            if (InRptr < InWptr) Sleep(3000);
            com_close();
            ExitThread(0);
        }
    }
}

//------------------------------------------------------------------------
// Output thread.   Output buffering greatly improves performance
//------------------------------------------------------------------------
DWORD WINAPI sc6551_output_thread(LPVOID param)
{
    int delay = 5;

    while(TRUE) {

        // Reset buffer and enable transmit
        OutWptr = OutBuf;
        StatReg |= StatTxE;

        // Poll for data to write
        while ((OutWptr-OutBuf) == 0) {
            // If heart beats have stopped terminate
            if ( (GetTickCount()-LastTickCount)>1000) sc6551_close();
            // Exit if a terminate signal
            if (WaitForSingleObject(hStopOutput,delay) != WAIT_TIMEOUT) {
                com_close();
                ExitThread(0);
            }
            // Greater delay greater latency but better performance
            if (delay<100) delay += 5;
        }

        // Disable transmit then short wait to prevent race
        StatReg &= ~StatTxE;
        Sleep(10);

        // If not write only mode flush the buffer
        if (AciaComMode != COM_MODE_READ) {
            char * ptr = OutBuf;
            int len = OutWptr - OutBuf;
//            PrintLogF("W%d ",len);
            if (len > OBUFSIZ/2) delay = 5;
            while (len > 0) {
                int cnt = com_write(ptr,len);
                if (cnt < 1) {
//                    OutWptr=OutBuf;
//                    len = 0;
                    break;  //TODO Deal with write error
                }
                ptr += cnt;
                len -= cnt;
            }
        }

    }
}

//------------------------------------------------------------------------
// Use Heartbeat (HSYNC) to time receives and interrupts
//------------------------------------------------------------------------
void sc6551_heartbeat()
{
    // Countdown to receive next byte
    if (HBtimer-- < 1) {
        HBtimer = BaudDelay[BaudRate];
        // Set RxF if there is data to receive
        if (InRptr < InWptr) {
            StatReg |= StatRxF;
            if (!(CmdReg & CmdRxI)) {
                StatReg |= StatIRQ;
                AssertInt(1,0);
//                PrintLogF("IRQ ");
            }
        }
    }

    // Thread keepalive counter
    LastTickCount=GetTickCount();

    return;
}

//------------------------------------------------------------------------
// Port Read
//    MPI calls port reads for each slot in sequence. The CPU gets the
//    first non zero reply. This works as long as port reads default to
//    zero for unmatched ports.
// -----------------------------------------------------------------------
unsigned char sc6551_read(unsigned char port)
{
    unsigned char data = 0; // Default zero
    switch (port) {
        // Read input data
        case 0x68:
            if (InRptr < InWptr) data = *InRptr++;
            StatReg &= ~StatRxF;
//            PrintLogF("r%02X ",data);
            break;
        // Read status register
        case 0x69:
            data = StatReg;
            StatReg &= ~StatIRQ;
//            PrintLogF("s%02X ",data);
            break;
        // Read command register
        case 0x6A:
            data = CmdReg;
            break;
        // Read control register
        case 0x6B:
            data = CtlReg;
            break;
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
//            PrintLogF("t%02X ",data);
            *OutWptr++ = data;
            // Clear TxE if output buffer full
            if ((OutWptr-OutBuf) >= OBUFSIZ) StatReg &= ~StatTxE;
            break;
        // Clear status
        case 0x69:
            StatReg = 0;
            break;
        // Write Command register
        case 0x6A:
//            PrintLogF("c%02X ",data);
            CmdReg = data;
            if (CmdReg & CmdDTR) {
                if (sc6551_initialized != 1) sc6551_init();
            } else {
                if (sc6551_initialized == 1) sc6551_close();
            }
            break;
        // Write Control register
        case 0x6B:
//            PrintLogF("n%02X ",data);
            CtlReg = data;
            BaudRate = CtlReg & CtlBaud;
            break;
    }
}
