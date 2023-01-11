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

#include "acia.h"
#include "sc6551.h"
#include "logger.h"

#undef MYDBG

#ifdef MYDBG
#define BP "%c%c%c%c%c%c%c%c"
#define BB(i) (i & 0x80 ? '1' : '0'), (i & 0x40 ? '1' : '0'), \
              (i & 0x20 ? '1' : '0'), (i & 0x10 ? '1' : '0'), \
              (i & 0x08 ? '1' : '0'), (i & 0x04 ? '1' : '0'), \
              (i & 0x02 ? '1' : '0'), (i & 0x01 ? '1' : '0')
char *PN[] = {"DAT","STA","CMD","CTL"};
#endif

//------------------------------------------------------------------------
// Handles and buffers for I/O threads
//------------------------------------------------------------------------

#define IBUFSIZ 512 //1024
DWORD WINAPI sc6551_input_thread(LPVOID);
HANDLE hInputThread;
HANDLE hStopInput;
char InBuf[IBUFSIZ];
char *InBufPtr = InBuf;
int InBufCnt = 0;

#define OBUFSIZ 1032
DWORD WINAPI sc6551_output_thread(LPVOID);
HANDLE hOutputThread;
HANDLE hStopOutput;
char OutBuf[OBUFSIZ];
char *OutBufPtr = OutBuf;
int OutBufFree = 0;

int sc6551_initialized = 0;

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

    if (sc6551_initialized==1) { PrintLogF("6551 already init\n"); return; } 
PrintLogF("\n");
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

PrintLogF("6551 init type:%d iport:%d cport:%d iomode:%d file:%s\n",
    AciaComType, AciaTcpPort, AciaComPort, AciaFileMode, AciaFilePath);
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

PrintLogF("6551 input thread %d\n",AciaFileMode);

    while(TRUE) {
        // If Write Mode reads return null characters
        if (AciaFileMode == FILE_WRITE) {
            *InBuf = 0; 
            InBufPtr = InBuf+1;
            InBufCnt = 1;
            if (WaitForSingleObject(hStopInput,1000) != WAIT_TIMEOUT)
                ExitThread(0);
        } else {
		    if (InBufCnt < 1) {     // Read if input buffer empty
                InBufCnt = com_read(InBuf,IBUFSIZ);
        	    if (InBufCnt < 0) {
PrintLogF("6551 Read ERROR");
				    Sleep(1000);    //TODO Handle com read error
				    InBufCnt = 0;
		    	}
                InBufPtr = InBuf;
            }
            if (WaitForSingleObject(hStopInput,50) != WAIT_TIMEOUT)
                ExitThread(0);
        }
    }
}

//------------------------------------------------------------------------
// Output thread sends the output buffer
//------------------------------------------------------------------------

DWORD WINAPI sc6551_output_thread(LPVOID param)
{

PrintLogF("6551 output thread %d\n",AciaFileMode);

    char * ptr;
    int towrite = 0;
    int written = 0;

    while(TRUE) {
        // If Read Mode writes go to the bit bucket 
        if (AciaFileMode == FILE_READ) {
            OutBufPtr = OutBuf;
            OutBufFree = OBUFSIZ;
            if (WaitForSingleObject(hStopOutput, 1000) != WAIT_TIMEOUT)
                ExitThread(0);
        } else {
            ptr = OutBuf;
            while (TRUE) {
                towrite = OutBufPtr - ptr;
                if (towrite < 1) break;
                written = com_write(ptr,towrite);
                if (written < 1) {
PrintLogF("6551 Write ERROR");
                    break;         // TODO handle com write error
                } else {
                    ptr = ptr + written;
                }
            }
            OutBufPtr = OutBuf;
            OutBufFree = OBUFSIZ;

            if (WaitForSingleObject(hStopOutput, 50) != WAIT_TIMEOUT) {
                if (OutBufFree == OBUFSIZ) ExitThread(0);
            }
        }
    }
}

//------------------------------------------------------------------------
// Use Heartbeat (HSYNC) to assert interrupts
//------------------------------------------------------------------------
void sc6551_ping()
{
    if (CmdReg & CmdDTR) {
        int irupt = 0;
        if ((InBufCnt > 0) && !(CmdReg & CmdRxI)) irupt = 1;
        if ((OutBufFree > 4) && ((CmdReg & CmdTIRB) == TIRB_On)) irupt = 1;
        if (irupt) {
            AssertInt(IRQ,0);
            StatReg |= StatIRQ;
        }
	}
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

PrintLogF("%02x ",data);

            break;

        // Read status register
        case 0x69:
            if (CmdReg & CmdDTR) {
				// StatRxF true if there is data in input buffer
                if (InBufCnt > 0) {
					StatReg |= StatRxF;
				} else {
					StatReg &= ~StatRxF;
                }
                // StatTxE true if space is left in output buffer
                if (OutBufFree > 8) {
					StatReg |= StatTxE;
                } else {
					StatReg &= ~StatTxE;
                }
            } else {
                StatReg = 0;
            }
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

#ifdef MYDBG
    if ((port > 0x67) && (port < 0x6C)) {
        PrintLogF("r %s %02x " BP "\n",
                    PN[port-0x68], data, BB(data));
    }
#endif

    return data;
}

//------------------------------------------------------------------------
// Port Write
//------------------------------------------------------------------------

void sc6551_write(unsigned char data,unsigned short port)
{

#ifdef MYDBG
    if ((port > 0x67) && (port < 0x6C)) {
        PrintLogF("w %s %02x " BP "\n",
                    PN[port-0x68], data, BB(data));
    }
    
#endif

    switch (port) {

        // Write data
        case 0x68:
            if (OutBufFree-- > 0) *OutBufPtr++ = data;
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

            if (CmdReg & CmdEcho) {    // not set by os9 t2 acia driver
                com_set(LOCAL_ECHO,1);
            } else {
                com_set(LOCAL_ECHO,0);
            }

            break;

        // Write Control register
        case 0x6B:
            CtlReg = data;
            break;
    }
}
