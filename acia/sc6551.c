/*
Copyright E J Jaquay 2022
This file is part of VCC (Virtual Color Computer).

VCC (Virtual Color Computer) is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version.

VCC (Virtual Color Computer) is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#include "acia.h"
#include "sc6551.h"

//------------------------------------------------------------------------
// Handles and buffers for I/O threads
//------------------------------------------------------------------------

#define IBUFSIZ 512
DWORD WINAPI sc6551_input_thread(LPVOID);
HANDLE hInputThread;
char InBuf[IBUFSIZ];
char *InBufPtr = InBuf;
int InBufCnt = 0;

#define OBUFSIZ 512
DWORD WINAPI sc6551_output_thread(LPVOID);
HANDLE hOutputThread;
char OutBuf[OBUFSIZ];
char *OutBufPtr = OutBuf;
int OutBufFree = 0;

//------------------------------------------------------------------------
//  Initiallize sc6551. This gets called when DTR is set
//------------------------------------------------------------------------
void sc6551_init()
{
    DWORD id;

    if (sc6551_initialized ) return;

    // Close any previous instance then open communications
    sc6551_close();
    com_open();

    // Create I/O threads
    hInputThread =  CreateThread(NULL,0,sc6551_input_thread, NULL,0,&id);
    hOutputThread = CreateThread(NULL,0,sc6551_output_thread,NULL,0,&id);

    // Clear status register and flush input buffer
    InBufCnt = 0;
    InBufPtr = InBuf;
    OutBufFree = OBUFSIZ;
    OutBufPtr = OutBuf;
    sc6551_initialized = 1;
}

//------------------------------------------------------------------------
//  Close sc6551.  This gets called when DTR is cleared
//------------------------------------------------------------------------
void sc6551_close()
{
    // Terminate I/O threads
    if (hInputThread) {
        TerminateThread(hInputThread,1);
        WaitForSingleObject(hInputThread,2000);
        hInputThread = NULL;
    }
    if (hOutputThread) {
        TerminateThread(hOutputThread,1);
        WaitForSingleObject(hOutputThread,2000);
        hOutputThread = NULL;
    }
    // Close communications
    com_close();

    sc6551_initialized = 0;
}

//------------------------------------------------------------------------
// Input thread loads the input buffer
//------------------------------------------------------------------------

DWORD WINAPI sc6551_input_thread(LPVOID param)
{
    while(TRUE) {
		if (InBufCnt < 1) {     // Read if input buffer empty
            InBufCnt = com_read(InBuf,IBUFSIZ);
        	if (InBufCnt < 0) {
				Sleep(1000);    //TODO Handle com read error
				InBufCnt = 0;
			}
            InBufPtr = InBuf;
        }
		Sleep(50);              // Poll for buffer emptied
    }
}

//------------------------------------------------------------------------
// Output thread sends the output buffer
//------------------------------------------------------------------------

DWORD WINAPI sc6551_output_thread(LPVOID param)
{
    char * ptr;
    int towrite = 0;
    int written = 0;

    while(TRUE) {
        ptr = OutBuf;
        while (TRUE) {
            towrite = OutBufPtr - ptr;
            if (towrite < 1) break;
            written = com_write(ptr,towrite);
            if (written < 1) {
                break;         // TODO handle com write error
            } else {
                ptr = ptr + written;
            }
        }
        OutBufPtr = OutBuf;
        OutBufFree = OBUFSIZ;
        Sleep(50);              // Poll every 0.05 sec for output
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
            if (InBufCnt-- > 0) data = *InBufPtr++;
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
                if (OutBufFree > 4) {
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
    }
    return data;
}

//------------------------------------------------------------------------
// Port Write
//------------------------------------------------------------------------

void sc6551_write(unsigned char data,unsigned short port)
{
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
                sc6551_init();
            } else {
                sc6551_close();
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

