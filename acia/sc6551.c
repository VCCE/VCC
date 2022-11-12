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
int  com_write(char);
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

HANDLE hEventRead;       // Event handle for read thread
HANDLE hEventWrite;      // Event handle for write thread

HANDLE hInputThread;
HANDLE hOutputThread;

char InData;
int  InDataRdy=0;
char OutData;
int  OutDataRdy=0;

//------------------------------------------------------------------------
//  Functions to manage status reg flags
//------------------------------------------------------------------------
CRITICAL_SECTION StatCritSec;
int StatCritSecInit=0;


void init_status_section() {
	if (!StatCritSecInit) {
		InitializeCriticalSection(&StatCritSec);
		StatCritSecInit = 1;
	}
}

void destroy_status_section() {
	if (StatCritSecInit) {
		DeleteCriticalSection(&StatCritSec);
		StatCritSecInit = 0;
	}
}

int set_stat_flag(unsigned char flag) 
{
	if (!StatCritSecInit) {
		InitializeCriticalSection(&StatCritSec);
		StatCritSecInit = 1;
	}
    EnterCriticalSection(&StatCritSec);
    int was_set = ( (StatReg & flag) != 0);
	StatReg = StatReg | flag;
    LeaveCriticalSection(&StatCritSec);
	return was_set;
}

int clr_stat_flag(unsigned char flag)
{
	if (!StatCritSecInit) {
		InitializeCriticalSection(&StatCritSec);
		StatCritSecInit = 1;
	}
    EnterCriticalSection(&StatCritSec);
    int was_clr = ( (StatReg & flag) == 0);
	StatReg = StatReg & ~flag;
    LeaveCriticalSection(&StatCritSec);
	return was_clr;
}

//------------------------------------------------------------------------
//  Initiallize sc6551. This gets called when DTR is set
//------------------------------------------------------------------------
void sc6551_init()
{
    DWORD id;

    // Close any previous instance then open communications
    sc6551_close();
    com_open();

    // Clear ready flags
  	InDataRdy=0;
	OutDataRdy=0;

    // Create input thread and event to wake it
    hInputThread=CreateThread(NULL,0,sc6551_input_thread,NULL,0,&id);
    hEventRead = CreateEvent (NULL,FALSE,FALSE,NULL);

    // Create output thread and event to wake it
    hOutputThread=CreateThread(NULL,0,sc6551_output_thread,NULL,0,&id);
    hEventWrite = CreateEvent (NULL,FALSE,FALSE,NULL);

//	InitializeCriticalSection(&StatCritSec);
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
//	DeleteCriticalSection(&StatCritSec);
    destroy_status_section();
    com_close();
    sc6551_initialized = 0;
}

//------------------------------------------------------------------------
// Output thread. Used to retry output (only if media is busy)
//------------------------------------------------------------------------
DWORD WINAPI sc6551_output_thread(LPVOID param)
{
	int tries;
    while(TRUE) {
		int ms = 5000; 
        WaitForSingleObject(hEventWrite,ms);
		tries = 0;
        while (OutDataRdy) {
			Sleep(50);
    		int rc = com_write(OutData); // returns zero if media is busy
            if (rc == 1) {
				set_stat_flag(StatTxE);
        		OutDataRdy = 0;
			} else if ((rc < 0) || tries > 50) {
        		OutDataRdy = 0;
				set_stat_flag(StatTxE|StatOvr);
			}
			tries++;
		}
    }
}

//------------------------------------------------------------------------
// Input thread 
//------------------------------------------------------------------------

// Data is sent one char at a time but input is buffered 
#define INBUFSIZ 128
char InBuffer[128];
char *InBufPtr = InBuffer;
int InBufCnt=0;

DWORD WINAPI sc6551_input_thread(LPVOID param)
{
    DWORD rc;
	int ms = 2000;    // maximum loop wait 
    while(TRUE) {
        rc = WaitForSingleObject(hEventRead,ms);
        if (CmdReg & CmdDTR) {
			// Assert in a fairly tight loop until the driver reads the data.
//            if ( InDataRdy ) {
//			    ms = 1;

			if (InBufCnt<1) {
				InBufPtr = InBuffer;
                InBufCnt = com_read(InBuffer,INBUFSIZ);
				ms = 3;				

//                com_read(&InData,1);
//                InDataRdy = 0;
//				set_stat_flag(StatIRQ);  // not needed for os9
//            } else if (rc == WAIT_TIMEOUT) {
				// is IRQ even enabled? 
                // if (CmdReg & CmdRxI) AssertInt(IRQ,0);
            } else {
				set_stat_flag(StatIRQ);  // not needed for os9
                AssertInt(IRQ,0);
				if (ms < 2000) ms += 1;
            }
        } else {
			ms = 2000;
		}
    }
}

//------------------------------------------------------------------------
// Port I/O
//------------------------------------------------------------------------
unsigned char sc6551_read(unsigned char port)
{
    unsigned char data;
    switch (port) {
		// Read data
        case 0x68:
            data = InData;
            InDataRdy = 1;
			clr_stat_flag(StatRxF);
            SetEvent(hEventRead); 
            break;

		// Read status register
        case 0x69:

//            if (InDataRdy == 0) { 
//				set_stat_flag(StatRxF);
//			} 
            if (InBufCnt > 0) {
				set_stat_flag(StatRxF);
				InData = *InBufPtr++;
				InBufCnt--;
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

void sc6551_write(unsigned char data,unsigned short port)
{
    switch (port) {
		// Write data
        case 0x68:
            // com_write returns 1=data read, 0=not blocked, -1=error
			int rc = com_write(data);
            if (rc == 1) {
				set_stat_flag(StatTxE);
			} else if (rc == 0) {
				clr_stat_flag(StatTxE);
				OutData = data;
	  			OutDataRdy = 1;
                SetEvent(hEventWrite);
			} else {
				set_stat_flag(StatTxE|StatOvr);
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
  				InDataRdy=1;  			// Ready for some input
				OutDataRdy=0; 			// Nothing to output yet 
				set_stat_flag(StatTxE);
                SetEvent(hEventRead);	// Waken input thread
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

// com_write blocking depends on the media.
// on success return 1, media busy 0; error -1
int com_write(char data) {
    switch (AciaComType) {
    case 0: // Legacy Console
        return console_write(&data,1);
    case 1:
        return wincmd_write(&data,1);
    }
    return 0;
}

// com_read is assumed to block until data is available
// on success return 1, error -1
int com_read(char * buf,int len) {  // returns bytes read
    switch (AciaComType) {
    case 0: // Legacy Console
        return console_read(buf,len);
    case 1:
        return wincmd_read(buf,len);
    }
    return 0;
}

