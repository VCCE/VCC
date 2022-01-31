
// Status register bits
// b0 parity error if true, self clearing 
// b1 frame error if true, self clearing
// b2 overrun error if true, self clearing
// b3 Rcv Data register full if true  (generates IRQ)
// b4 Snd Data register empty if true (generates IRQ)
// b5 DCD Data carrier detect (change generates IRQ)
// b6 DSR Data set ready      (chante generates IRQ)
// b7 IRQ true = interrupt generated, cleared on read statreg

// Command register bits
// b0 DTR 0=disable receiver and all interupts 1=enable reciever and interrupts
// b1 Reciever IRQ enable, 0 = IRQ enabled per bit 3 of status reg.  1=disabled
// b3 b2
// 0  0 transmit IRQ disabled, RTS high, transmitter off
// 0  1 transmit IRQ enabled,  RTS low,  transmitter on
// 1  0 transmit IRQ disabled, RTS low,  tranmitter on
// 1  1 transmit IRQ disabled, RTS low,  transmit break
// B4 Echo mode. 0 = normal 1 = Echo (B2&B3 zero)
// B7 B6 B4
// -  -  0  Parity disabled
// 0  0  1  Odd parity recieve and transmit
// 0  1  1  Even parity recieve and transmit
// 1  0  1  Mark Parity transmitted, check disabled
// 1  1  1  Space parity, check disabled

unsigned char StatReg = 0; // Tells processor status of ACIA functions
#define StatPar  0x01
#define StatFrm  0x02
#define StatOvr  0x04

#define StatDCD  0x20 
#define StatDSR  0x40 

#define StatRxF  0x08
#define StatTxE  0x10
#define StatIRQ  0x80

unsigned char CmdReg  = 0; // Controls transmit recieve functions
#define CmdDTR   0x01
#define CmdRxI   0x02
#define CmdTIRB  0x0C
#define CmdEcho  0x10   
#define CmdPar   0xE0

unsigned char CtlReg  = 0; // Selects  word length, stop bits, baud rate
unsigned char Output_buf = 0; // Send data register
unsigned char Input_buf = 0;  // Recieve data register

void acia_open_com();
int acia_read_com();
int acia_write_com();
void acia_close_com();

DWORD WINAPI acia_worker(LPVOID);
void acia_worker_init();
//void acia_log(const void *, ...);
unsigned char acia_read(unsigned char);
void acia_write(unsigned char,unsigned short);

HANDLE hWorker = NULL; 
DWORD  ThreadId;
HANDLE hComIn;
HANDLE hComOut;
HANDLE hEvents[2];
int	worker_initialized = 0;
int comtype = 0;  // 0 = console


// Open com
void acia_open_com() {
	switch (comtype) {
	case 0: // Console
		DWORD console_mode;
		AllocConsole();

		hComIn=GetStdHandle(STD_INPUT_HANDLE);
		GetConsoleMode(hComIn,&console_mode);
		console_mode &= ~ENABLE_PROCESSED_INPUT;  // disable ^C processing
		console_mode &= ~ENABLE_LINE_INPUT;       // disable line mode
		SetConsoleMode(hComIn,console_mode);
		break;
	}
}

// read com
int acia_read_com() {
	int retval = 0;
	switch (comtype) {
	case 0: // Console
		ReadConsole(hComIn,&Input_buf,1,(LPDWORD)&retval,NULL);
		break;
	}
	return retval;
}

// write com
int acia_write_com() {
	int retval = 0;
	switch (comtype) {
	case 0: // Console
		WriteConsole(hComOut,&Output_buf,1,(LPDWORD)&retval,NULL);
		break;
	}
	return retval;
}

// close com
void acia_close_com() {
	switch (comtype) {
	case 0: // Console
		FreeConsole();
		break;
	}
}

// Worker thread reads data from input.
DWORD WINAPI 
acia_worker(LPVOID param) 
{ 
	DWORD dw;
	int ctr=0;
	while(true) {
    	dw = WaitForMultipleObjects(2,hEvents,FALSE,250);
		if (CmdReg & CmdDTR) {
			if (!(StatReg & StatRxF)) { 
				acia_read_com();
				StatReg = StatReg | StatRxF; // mark buf full
				Sleep(1); // Give Coco time to be ready for interrupt
			}
			StatReg = StatReg | StatIRQ; 
			CPUAssertInterupt(IRQ,0);
		}
	}
}

void 
acia_worker_init()
{
	if (worker_initialized == 0) {
	    acia_open_com();
		AllocConsole();
		hComIn=GetStdHandle(STD_INPUT_HANDLE);
		FlushConsoleInputBuffer(hComIn);
		hComOut=GetStdHandle(STD_OUTPUT_HANDLE);
		hWorker= CreateThread(NULL,0,acia_worker,NULL,0,&ThreadId);
		hEvents[1] = CreateEvent (NULL,FALSE,FALSE,NULL);
		hEvents[0] = CreateEvent (NULL,FALSE,FALSE,NULL);
		worker_initialized = 1;
	}
}

// Log - Put formatted string to the console
void 
acia_log(const void * fmt, ...)
{
	unsigned long dummy;
	va_list args;
	char str[512];
	va_start(args, fmt);
	vsnprintf(str, 512, (char *)fmt, args);
	va_end(args);
	WriteConsole(hComOut,str,strlen(str),&dummy,0);
}

unsigned char acia_read(unsigned char port)
{
	unsigned char data;
	switch (port) {
		case 0x68:
			data = Input_buf;
			StatReg = StatReg & ~StatRxF;  // mark buffer empty
			SetEvent(hEvents[0]);          // tell worker it is
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

void acia_write(unsigned char data,unsigned short port)
{
	acia_worker_init(); // Until we have some other way to init

	switch (port) {
		case 0x68:
		    Output_buf = data;
			acia_write_com();
			StatReg = StatReg | StatTxE;  // mark out buffer empty
			break;

		case 0x69:
			StatReg = 0;
			break;

		case 0x6A:
			CmdReg = data;
			// If DTR wake up worker
		    if (CmdReg & CmdDTR) {
			    StatReg = StatTxE;       // mark i/o buffers empty
				SetEvent(hEvents[1]);
			} else {
				StatReg = 0;
			}
			break;

		case 0x6B:
			CtlReg = data;
			break;
    }
}
