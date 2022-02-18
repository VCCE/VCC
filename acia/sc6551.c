
#include "acia.h"

//------------------------------------------------------------------------
// sc6551
//------------------------------------------------------------------------

// sc6551 private functions 

DWORD WINAPI sc6551_input_thread(LPVOID);
void sc6551_init();
void sc6551_close();


unsigned char StatReg = 0;
// Status register bits.  
// b0 parity error if true, self clearing 
// b1 frame error if true, self clearing
// b2 overrun error if true, self clearing
// b3 Rcv Data register full if tru
// b4 Snd Data register empty if true
// b5 DCD Data carrier detect
// b6 DSR Data set ready
// b7 IRQ Interrupt generated
// Only RxF, TxE, and IRQ are used here
#define StatRxF  0x08
#define StatTxE  0x10
#define StatIRQ  0x80

unsigned char CmdReg  = 0;
// Command register bits.
// b0 DTR 1 = enable receive and interupts
// b1 Receiver IRQ enable.
// b3 b2
// 0  0 transmit IRQ disabled, RTS high, transmitter off
// 0  1 transmit IRQ enabled,  RTS low,  transmitter on
// 1  0 transmit IRQ disabled, RTS low,  tranmitter on
// 1  1 transmit IRQ disabled, RTS low,  transmit break
// B4 Echo mode. 0 = normal 1 = Echo (B2&B3 zero)
// B7 B6 B4
// -  -  0  Parity disabled
// 0  0  1  Odd parity receive and transmit
// 0  1  1  Even parity receive and transmit
// 1  0  1  Mark Parity transmitted, check disabled
// 1  1  1  Space parity, check disabled
// Only DTR is used here
#define CmdDTR   0x01

// CtrReg is not really used, it is just echoed back to CoCo
unsigned char CtlReg  = 0;

// Data register is loaded by input thread.
unsigned char RcvChr = 0;

HANDLE hEvents[3];
enum {
    EVT_DAT_REG_READ,      // Data register has been read 
    EVT_CMD_REG_WRITE,     // Command register was written
    EVT_CMD_TERMINATE      // Ask thread to terminate
};

int comtype = 0;           // Default 0 = console
int	sc6551_initialized = 0;

//------------------------------------------------------------------------
//  Initiallize sc6551 
//------------------------------------------------------------------------

void sc6551_init()
{
    DWORD id;
	if (sc6551_initialized == 0) {
	    acia_open_com();
		CreateThread(NULL,0,sc6551_input_thread,NULL,0,&id);
		hEvents[2] = CreateEvent (NULL,FALSE,FALSE,NULL);
		hEvents[1] = CreateEvent (NULL,FALSE,FALSE,NULL);
		hEvents[0] = CreateEvent (NULL,FALSE,FALSE,NULL);
		sc6551_initialized = 1;
	}
}

void sc6551_close()
{
	SetEvent(hEvents[EVT_CMD_TERMINATE]);
	acia_close_com();
	sc6551_initialized = 0;
}

//------------------------------------------------------------------------
// Input Thread.
//------------------------------------------------------------------------

// Reads hang. They are done in a seperate thread.
DWORD WINAPI sc6551_input_thread(LPVOID param) 
{ 
	DWORD dw;
	while(TRUE) {
    	dw = WaitForMultipleObjects(2,hEvents,FALSE,250);
		if (CmdReg & CmdDTR) {
			if (!(StatReg & StatRxF)) { 
				RcvChr = acia_read_com();    // Store char read
				StatReg = StatReg | StatRxF; // mark RcvChr full
			} 
		    Sleep(1); // Give Coco time to be ready for interrupt
			StatReg = StatReg | StatIRQ; 
			AssertInt(IRQ,0);
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
		case 0x68:
			data = RcvChr;
			StatReg = StatReg & ~StatRxF;         // mark buffer empty
			SetEvent(hEvents[EVT_DAT_REG_READ]);  // tell input worker
			break;
		case 0x69:
			data = StatReg;
    		StatReg = StatReg & ~StatIRQ;   // Stat read clears IRQ flag
			break;
		case 0x6A:
			data = CmdReg;
			break;
		case 0x6B:
			data = CtlReg;
			break;
    }
	return data;
}

void sc6551_write(unsigned char data,unsigned short port)
{
	switch (port) {
		case 0x68:
			acia_write_com(data);
			StatReg = StatReg | StatTxE;  // mark out buffer empty
			break;
		case 0x69:
			StatReg = 0;
			break;
		case 0x6A:
			CmdReg = data;
            // If DTR set enable sc6551
		    if (CmdReg & CmdDTR) {
                sc6551_init();
			    StatReg = StatTxE;                     // mark i/o buffers empty
				SetEvent(hEvents[EVT_CMD_REG_WRITE]);  // Tell input worker
			} else {
            // Else disable sc6551
                sc6551_close();
				StatReg = 0;
			}
			break;
		case 0x6B:
			CtlReg = data;  // Not used, just returned on read
			break;
    }
}

