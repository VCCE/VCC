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

#include "acia.h"

//------------------------------------------------------------------------
// sc6551
//------------------------------------------------------------------------

// sc6551 private functions
DWORD WINAPI sc6551_input_thread(LPVOID);
DWORD WINAPI sc6551_output_thread(LPVOID);
void sc6551_output(char);

// Comunications hooks
void com_open();
void com_close();
int  com_write(char*,int);
int  com_read(char*,int);

// Stat Reg is protected by a critical section to prevent 
// race conditions with I/0 threads
unsigned char StatReg;

// Status register bits.
// b0 Par Rx parity eror
// b1 Frm Rx framing error
// B2 Ovr Rx data over run error
// b3 RxF Rx Data register full
// b4 TxE Tx Data register empty
// b5 DCD Data carrier detect
// b6 DSR Data set ready
// b7 IRQ Interrupt generated
// Only RxF, TxE, and IRQ are used here
#define StatOvr  0x04
#define StatRxF  0x08
#define StatTxE  0x10
#define StatIRQ  0x80

unsigned char CmdReg  = 0;
// Command register bits.
// b0   DTR Enable receive and interupts
// b1   RxI Receiver IRQ enable.
// b2-3 TIRB Transmit IRQ control
// B4   Echo Set=Activated
// B5-7 Par  Parity control
#define CmdDTR  0x01
#define CmdRxI  0x02

// CtrReg is not used (yet), it is just echoed back to CoCo
unsigned char CtlReg = 0;

//HANDLE hEventRead;       // Event handle for read thread
//HANDLE hEventWrite;      // Event handle for write thread

HANDLE hInputThread;
HANDLE hOutputThread;

char InData;
//int  InDataRdy=0;
char OutData;
int  OutDataRdy=0;

//------------------------------------------------------------------------
//  Functions to manage status reg flags
//------------------------------------------------------------------------
CRITICAL_SECTION StatCritSec;
int StatCritSecInit=0;


void init_status_section() {
//	if (!StatCritSecInit) {
//		InitializeCriticalSection(&StatCritSec);
//		StatCritSecInit = 1;
//	}
}

void destroy_status_section() {
//	if (StatCritSecInit) {
//		DeleteCriticalSection(&StatCritSec);
//		StatCritSecInit = 0;
//	}
}

int set_stat_flag(unsigned char flag) 
{
//	if (!StatCritSecInit) {
//		InitializeCriticalSection(&StatCritSec);
//		StatCritSecInit = 1;
//	}
//   EnterCriticalSection(&StatCritSec);
    int was_set = ( (StatReg & flag) != 0);
	StatReg = StatReg | flag;
//    LeaveCriticalSection(&StatCritSec);
	return was_set;
}

int clr_stat_flag(unsigned char flag)
{
//	if (!StatCritSecInit) {
//		InitializeCriticalSection(&StatCritSec);
//		StatCritSecInit = 1;
//	}
 //   EnterCriticalSection(&StatCritSec);
    int was_clr = ( (StatReg & flag) == 0);
	StatReg = StatReg & ~flag;
//    LeaveCriticalSection(&StatCritSec);
	return was_clr;
}

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

    // Clear ready flags
  	//InDataRdy=0;
	OutDataRdy=0;

    // Create threads 
    hInputThread=CreateThread(NULL,0,sc6551_input_thread,NULL,0,&id);
    hOutputThread=CreateThread(NULL,0,sc6551_output_thread,NULL,0,&id);
    
    init_status_section();

    sc6551_initialized = 1;
}

//------------------------------------------------------------------------
//  Close sc6551.  This gets called when DTR is cleared
//------------------------------------------------------------------------
void sc6551_close()
{
    if (hInputThread) {
        TerminateThread(hInputThread,1);
        WaitForSingleObject(hInputThread,2000);
    }
    if (hOutputThread) {
		TerminateThread(hOutputThread,1);
		WaitForSingleObject(hOutputThread,2000);
    }
    hInputThread = NULL;
    hOutputThread = NULL;
    destroy_status_section();
    com_close();
    sc6551_initialized = 0;
}

//------------------------------------------------------------------------
// Input thread loads the input buffer if it is empty
//------------------------------------------------------------------------

#define INBUFSIZ 128
char InBuffer[INBUFSIZ];
char *InBufPtr = InBuffer;
int InBufCnt=0;

DWORD WINAPI sc6551_input_thread(LPVOID param)
{
	int ms;   
    while(TRUE) {
        if (CmdReg & CmdDTR) {
			ms = 1;         // Read will block once buffer is cleared
			if (InBufCnt<1) {
                InBufCnt = com_read(InBuffer,INBUFSIZ);		
				InBufPtr = InBuffer;
            } else {
				set_stat_flag(StatIRQ);  // not really needed for os9
                AssertInt(IRQ,0);        // Assert until buffer empty
            }
        } else {
			ms = 2000;
		}
		Sleep(ms);
    }
}

//------------------------------------------------------------------------
// Output thread empties the output buffer 
//------------------------------------------------------------------------
#define OUTBUFSIZ 4000 
char OutBuffer[OUTBUFSIZ];
char *OutBufPtr = OutBuffer;

DWORD WINAPI sc6551_output_thread(LPVOID param)
{
	char * ptr;
	int towrite = 0;
	int written = 0;

    while(TRUE) {
        if (CmdReg & CmdDTR) {
			ptr = OutBuffer;
			while (TRUE) {
				towrite = OutBufPtr - ptr;
				if (towrite < 1) break;
				written = com_write(ptr,towrite);
				if (written < 1) {
					// deal with write error somehow
					break;
				} else {
					ptr = ptr + written;
				}
				Sleep(5);
			}
			OutBufPtr = OutBuffer;
			Sleep(50);
		} else {
			Sleep(1000);
		}
    }
}

//------------------------------------------------------------------------
// Port I/O
//------------------------------------------------------------------------
void sc6551_write(unsigned char data,unsigned short port)
{
    switch (port) {
		// Write data
        case 0x68:
			if (OutBufPtr < (OutBuffer + OUTBUFSIZ + 1)) {
				*OutBufPtr++ = data;
			}
            break;
		// Clear status
        case 0x69:
            StatReg = 0;
            break;
		// Set Command register
        case 0x6A:
            CmdReg = data;
            // If DTR set enable sc6551
            if (CmdReg & CmdDTR) {
                if (sc6551_initialized == 0) sc6551_init();
            // Else disable sc6551
            } else {
                sc6551_close();
                StatReg = 0;
            }
            break;
        // Set Control register
        case 0x6B:
            CtlReg = data;  // Not used, just returned on read
            break;
    }
}

unsigned char sc6551_read(unsigned char port)
{
    unsigned char data;
    switch (port) {
		// Read data
        case 0x68:
            data = InData;
			clr_stat_flag(StatRxF);
            break;

		// Read status register
        case 0x69:
            if (CmdReg & CmdDTR) {
			    // Check for data in input data
            	if (InBufCnt > 0) {
					set_stat_flag(StatRxF);
					InData = *InBufPtr++;
					InBufCnt--;
				}
			    // Check for plenty of room in output buffer 
				if (OutBufPtr < (OutBuffer + OUTBUFSIZ - OUTBUFSIZ/4 )) {
					set_stat_flag(StatTxE);
				} else {
					clr_stat_flag(StatTxE);
				}
			}
            data = StatReg; 
			clr_stat_flag(StatIRQ);
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

//----------------------------------------------------------------
// Dispatch I/0 to communication type used.
// Hooks allow sc6551 to do communications to selected media
//----------------------------------------------------------------

// Open com
void com_open() {
    switch (AciaComType) {
    case 0: // Legacy Console
        console_open();
        break;
    case 1:
        wincmd_open();
        break;
    }
}

void com_close() {
    switch (AciaComType) {
    case 0: // Console
        console_close();
        break;
    case 1:
        wincmd_close();
        break;
    }
}

// com_write is assumed to block until some data is written
int com_write(char * buf, int len) {
    switch (AciaComType) {
    case 0: // Legacy Console
        return console_write(buf,len);
    case 1:
        return wincmd_write(buf,len);
    }
    return 0;
}

// com_read is assumed to block until some data is read
int com_read(char * buf,int len) {  // returns bytes read
    switch (AciaComType) {
    case 0: // Legacy Console
        return console_read(buf,len);
    case 1:
        return wincmd_read(buf,len);
    }
    return 0;
}

