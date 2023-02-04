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
// Inline interlock functions.  Used to coordinate writer thread.
//------------------------------------------------------------------------

#define SetIlock(l) (_InterlockedOr(&l,1) == 0)
#define ClrIlock(l) (_InterlockedAnd(&l,0) == 1)

//------------------------------------------------------------------------
// Handles and buffers for I/O threads
//------------------------------------------------------------------------

// sc6551 registers
unsigned char StatReg;
unsigned char CmdReg;
unsigned char CtlReg;

// Input 
#define IBUFSIZ 1024
DWORD WINAPI sc6551_input_thread(LPVOID);
char InBuf[IBUFSIZ];
char *InRptr = InBuf;
int Icnt = 0;

// Output 
#define OBUFSIZ 1024
DWORD WINAPI sc6551_output_thread(LPVOID);
char OutBuf[OBUFSIZ];
char *OutWptr = OutBuf;
int Wcnt = 0;
unsigned int volatile W_Ilock;

int sc6551_initialized = 0;

// BaudRate and Heartbeat counter used to pace I/O 
int BaudRate = 0;
int HBcounter = 0;

// BaudDelay table sets HBcounter.  These are approximate.
// If accuracy becomes an issue could use audio sample timer.
int BaudDelay[16] = { 0, 1000, 666, 454, 370, 333, 166,
                      83, 41, 26, 20, 13, 10, 6, 5, 1 };

// Baud rates: EXT, 50, 75, 110, 135, 150, 300, 600, 1200,
//             1800, 2400, 3600, 4800, 7200, 9600, 19200 

//------------------------------------------------------------------------
//  Nicely terminate an I/O thread
//------------------------------------------------------------------------
void sc6551_terminate_thread(HANDLE hthread, HANDLE hstop)
{
    if (hthread) {
        // Tell thread to exit
        SetEvent(hstop);
        // If that fails force it
        if (WaitForSingleObject(hthread,500) == WAIT_TIMEOUT) {
            //PrintLogF("FORCE TERMINATE %d\n",hthread);
            TerminateThread(hthread,1);
            WaitForSingleObject(hthread,500);
        }
        CloseHandle(hthread);
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

    // Create events to control them
    hStopInput  = CreateEvent(NULL,TRUE,FALSE,NULL);
    hStopOutput = CreateEvent(NULL,TRUE,FALSE,NULL);

    // Clear status register and mark buffers empty
    //InRptr = InBuf; InWptr = InBuf;
    InRptr = InBuf;
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
    CloseHandle(hStopInput);
    CloseHandle(hStopOutput);
    com_close();
    sc6551_initialized = 0;
}
//------------------------------------------------------------------------
// Input thread
//------------------------------------------------------------------------

DWORD WINAPI sc6551_input_thread(LPVOID param)
{
    int delay = 20;

    //PrintLogF("START-IN %d\n",hInputThread);

    Icnt = 0;
    while(TRUE) {
        if (Icnt == 0) {
            InRptr = InBuf;
            if (AciaComMode == COM_MODE_WRITE) {
                *InBuf = '\0';
                Icnt = 1;
                delay = 100;
            } else {
                int cnt = com_read(InBuf,IBUFSIZ);
                if (cnt < 1) {
                    // EOF regardless TODO User config
                    cnt = 2;
                    InBuf[0] = '\r';
                    InBuf[1] = EOFCHR;
                }
                //PrintLogF("R %d\n",cnt);
                Icnt = cnt;
                delay = (Icnt == 0) ? 200 : 0;
            }
        }
        if (WaitForSingleObject(hStopInput,delay) != WAIT_TIMEOUT) {
            if (Icnt > 0) Sleep(1000);
            //PrintLogF("TERMINATE-IN\n");
            ExitThread(0);
        }
    }
}

//------------------------------------------------------------------------
// Output thread.   
//------------------------------------------------------------------------
DWORD WINAPI sc6551_output_thread(LPVOID param)
{

    Wcnt = 0;
    OutWptr = OutBuf;

    //PrintLogF("START-OUT %d\n",hOutputThread);

    while(TRUE) {
        if (Wcnt > 0) {
            // Need interlock for TxE, OutWptr, and Wcnt
            if (SetIlock(W_Ilock)) {
                StatReg &= ~StatTxE;
                if (AciaComMode != COM_MODE_READ) {
                    char * ptr = OutBuf;
                    while (Wcnt > 0) {
                        int cnt = com_write(ptr,Wcnt);
                        //PrintLogF("W %d\n",cnt);
                        if (cnt < 1) break;  //TODO Deal with write error
                        Wcnt -= cnt;
                        ptr += cnt;
                    }
                }
                Wcnt = 0;
                OutWptr = OutBuf;
                ClrIlock(W_Ilock);
            }
        }

        int rc = WaitForSingleObject(hStopOutput,100);
        if (rc != WAIT_TIMEOUT) {
            //PrintLogF("TERMINATE-OUT\n");
            ExitThread(0);
        }
    }
}

//------------------------------------------------------------------------
// Use Heartbeat (HSYNC) to time receives and interrupts
// If accuracy becomes an issue could use audio sample timer instead.
//------------------------------------------------------------------------

void sc6551_heartbeat()
{
	// Countdown to receive next byte
    if (HBcounter-- < 1) {
        HBcounter = BaudDelay[BaudRate];

        // Set RxF if there is data in buffer
        if (Icnt) {
            StatReg |= StatRxF;
            if (!(CmdReg & CmdRxI)) {
                StatReg |= StatIRQ;
                AssertInt(1,0);
                //PrintLogF("IRQ ");
            }
        }

        // Set TxE if write buffer not full (interlocked)
        if (SetIlock(W_Ilock)) {
            if (Wcnt < OBUFSIZ) StatReg |= StatTxE;
            ClrIlock(W_Ilock);
        }
    }
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
            if (Icnt) {
                data = *InRptr++,
                Icnt--;
            }
            StatReg &= ~StatRxF;
            break;
        // Read status register
        case 0x69:
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
            StatReg &= ~StatTxE;
            *OutWptr++ = data;
            Wcnt++;
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
            break;
        // Write Control register
        case 0x6B:
            CtlReg = data;
            BaudRate = CtlReg & CtlBaud;
            //PrintLogF("Baud rate: %d\n",BaudRate);
            break;
    }
}
