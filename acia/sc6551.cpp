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
//
//------------------------------------------------------------------

#include "acia.h"
#include "sc6551.h"
#include <vcc/core/interrupts.h>
#include <vcc/utils/logger.h>
#include <atomic>

//------------------------------------------------------------------------
// Handles and buffers for I/O threads
//------------------------------------------------------------------------

HANDLE hInputThread;
HANDLE hOutputThread;
HANDLE hStopInput;
HANDLE hStopOutput;

// sc6551 registers
unsigned char StatReg;
unsigned char CmdReg;
unsigned char CtlReg;

// Input
constexpr auto IBUFSIZ = 1024u;
DWORD WINAPI sc6551_input_thread(LPVOID);
char InBuf[IBUFSIZ];
char *InRptr = InBuf;
int Icnt = 0;

// Output
constexpr auto OBUFSIZ = 1024u;
DWORD WINAPI sc6551_output_thread(LPVOID);
char OutBuf[OBUFSIZ];
char *OutWptr = OutBuf;
int Wcnt = 0;

// Atomic flag for interlock
std::atomic_flag Ilock{};

int sc6551_opened = 0;

unsigned int HBcounter = 0; // used to pace I/O

unsigned int IntClock;
static unsigned int DataLen;
static unsigned int StopBits;
static unsigned int EchoOn;
static unsigned int EnParity;
static unsigned int Parity;
static unsigned int BaudRate;

// BaudDelay table for supported rates.  These are approximate.
// If accuracy becomes an issue could use audio sample timer.
// Corresponds with {    X,  75,  75, 110, 300, 300, 300,  600,
//                    1200,2400,2400,4800,4800,9600,9600,19200 };

int BaudDelay[16] = {   0, 620, 620, 500, 250, 250, 250,  125,
                       60,  30,  30,  15,  15,   7,   7,    3 };

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
            DLOG_F("FORCE TERMINATE %d\n",hthread);
            TerminateThread(hthread,1);
            WaitForSingleObject(hthread,500);
        }
        CloseHandle(hthread);
        hthread = nullptr;
    }
}

//------------------------------------------------------------------------
//  Init sc6551.  This gets called when the DLL is loaded
//------------------------------------------------------------------------
void sc6551_init()
{
    IntClock = 0;
    DataLen  = 0;
    StopBits = 0;
    EchoOn   = 0;
    EnParity = 0;
    Parity   = 0;
    BaudRate = 0;
}

//------------------------------------------------------------------------
//  Open sc6551.  This gets called when DTR is raised.
//------------------------------------------------------------------------
void sc6551_open()
{
    DWORD id;

    if (sc6551_opened == 1) return;

    // Open comunication
    com_open();

    // Create I/O threads
    hInputThread =  CreateThread(nullptr,0,sc6551_input_thread, nullptr,0,&id);
    hOutputThread = CreateThread(nullptr,0,sc6551_output_thread,nullptr,0,&id);

    // Create events to control them
    hStopInput  = CreateEvent(nullptr,TRUE,FALSE,nullptr);
    hStopOutput = CreateEvent(nullptr,TRUE,FALSE,nullptr);

    // Clear status register and mark buffers empty
    InRptr = InBuf;
    OutWptr = OutBuf;
    Icnt = 0;
    Wcnt = 0;

    sc6551_opened = 1;
}

//------------------------------------------------------------------------
//  Close sc6551. This gets called when DTR is cleared
//------------------------------------------------------------------------
void sc6551_close()
{
    if (sc6551_opened) {
        sc6551_terminate_thread(hInputThread, hStopInput);
        sc6551_terminate_thread(hOutputThread, hStopOutput);
        CloseHandle(hStopInput);
        CloseHandle(hStopOutput);
        com_close();
        Icnt = 0;
        Wcnt = 0;
        sc6551_opened = 0;
    }
}

//------------------------------------------------------------------------
// Input thread
//------------------------------------------------------------------------
DWORD WINAPI sc6551_input_thread(LPVOID /*param*/)
{
    DLOG_F("START-IN %d\n",hInputThread);
    Icnt = 0;
    while(TRUE) {
        if (Icnt == 0) {
            InRptr = InBuf;
            if (AciaComMode == COM_MODE_WRITE) {
                *InBuf = '\0';
                Icnt = 1;
            } else {
                int cnt = com_read(InBuf,IBUFSIZ);
                if (cnt < 1) {
                    // EOF regardless TODO User config
                    cnt = 2;
                    InBuf[0] = '\r';
                    InBuf[1] = EOFCHR;
                }
                DLOG_F("R %d\n",cnt);
                Icnt = cnt;
            }
        } else {
            if (WaitForSingleObject(hStopInput,100) != WAIT_TIMEOUT) {
                if (Icnt > 0) Sleep(1000);
                DLOG_F("TERMINATE-IN\n");
                ExitThread(0);
            }
        }
    }
}

//------------------------------------------------------------------------
// Output thread.
//------------------------------------------------------------------------
DWORD WINAPI sc6551_output_thread(LPVOID /*param*/)
{
    Wcnt = 0;
    OutWptr = OutBuf;

    DLOG_F("START-OUT %d\n",hOutputThread);

    while(TRUE) {
        if (Wcnt > 0) {
            // Need interlock for TxE, OutWptr, and Wcnt
            if (!Ilock.test_and_set()) {
                StatReg &= ~StatTxE;
                if (AciaComMode != COM_MODE_READ) {
                    const char * ptr = OutBuf;
                    while (Wcnt > 0) {
                        int cnt = com_write(ptr,Wcnt);
                        DLOG_F("W %d\n",cnt);
                        if (cnt < 1) break;  //TODO Deal with write error
                        Wcnt -= cnt;
                        ptr += cnt;
                    }
                }
                Wcnt = 0;
                OutWptr = OutBuf;
                Ilock.clear();
            }
        }

        int rc = WaitForSingleObject(hStopOutput,100);
        if (rc != WAIT_TIMEOUT) {
            DLOG_F("TERMINATE-OUT\n");
            ExitThread(0);
        }
    }
}

//------------------------------------------------------------------------
// Use Heartbeat (HSYNC) to time receives and interrupts
// If accuracy becomes an issue could use audio sample timer instead.
//
// StatRxF 0x08 Data ready to read;
// StatTxE 0x10 Buffer ready to write;
// StatDCD 0x20 Data Carier detected if clr
// StatDSR 0x40 Data set Ready if clr
// StatIRQ 0x80 IRQ set
//
//------------------------------------------------------------------------

void sc6551_heartbeat()
{
    // Countdown to receive next byte
    if (HBcounter-- < 1) {
        HBcounter = BaudDelay[BaudRate];
        // Set RxF if there is data buffered
        if (Icnt) {
            StatReg |= StatRxF;
            // Assert CART if interrupts not disabled or already asserted.
            if (!((CmdReg & CmdRxI) || (StatReg & StatIRQ))) {
                AssertInt(gHostKey, INT_CART, IS_NMI);
                StatReg |= StatIRQ;
            }
        }
        // Set TxE if write buffer not full (interlocked)
        if (!Ilock.test_and_set()) {
            if (Wcnt < OBUFSIZ) StatReg |= StatTxE; //0x10
            Ilock.clear();
        }
    }
    return;
}

//------------------------------------------------------------------------
// Port Read
//    MPI calls port reads for each slot in sequence. The CPU gets the
//    first non zero reply if any.
// -----------------------------------------------------------------------

unsigned char sc6551_read(unsigned char port)
{
    unsigned char data = 0;      // Default zero
    switch (port-AciaBasePort) {
    // Read data
    case 0:
        // If data avail and RxF is set return the data
        if ((Icnt > 0) && (StatReg & StatRxF)) {
            data = *InRptr++;
            Icnt--;
        }
        // Clear RxF until timer resets it
        StatReg &= ~StatRxF;  //0x08
        break;
    // Read status register
    case 1:
        data = StatReg;
        StatReg &= ~StatIRQ;
        break;
    // Read command register
    case 2:
        data = CmdReg;
        break;
    // Read control register
    case 3:
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
    switch (port-AciaBasePort) {
    // Data
    case 0:
        StatReg &= ~StatTxE; //0x10
        *OutWptr++ = data;
        Wcnt++;
        break;
    // Write status does a reset
    case 1:
        DLOG_F("Reset\n");
        StatReg = 0;
        break;
    // Write Command register
    case 2:
        CmdReg = data;
        EchoOn = (CmdReg & 0x10) >> 4;
        Parity = (CmdReg & 0xE0) >> 5;
        EnParity = Parity & 1;
        Parity = Parity >> 1;
        if (CmdReg & CmdDTR) {
            sc6551_open();
        } else {
            sc6551_close();
        }
        break;
    // Write Control register
    case 3:
        CtlReg = data;
        BaudRate = CtlReg & 0x0F;
        IntClock = (CtlReg & 0x10) >> 4;
        DataLen  = 8 - ((CtlReg & 0x60) >> 5);
        StopBits = (((CtlReg & 0x80) >> 7) == 0) ? 0 : 2;
        break;
    }
}
