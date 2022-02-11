#include <conio.h>

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
unsigned char Output_chr = 0; // Send data register
unsigned char Input_chr = 0;  // Recieve data register

void acia_open_com();
void acia_read_com();
void acia_write_com();
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

    // Legacy console is kind of ugly but it is used because the 
    // windows psuedo console only works on win 10 and 11.
    // Another possibility is curses but that requires cygwin dll
	case 0: // Legacy Console
		DWORD console_mode;
		AllocConsole();
		hComIn=GetStdHandle(STD_INPUT_HANDLE);
		GetConsoleMode(hComIn,&console_mode);
		console_mode &= ~ENABLE_PROCESSED_INPUT;    // disable ^C processing
		console_mode &= ~ENABLE_LINE_INPUT;         // disable line mode
	    SetConsoleMode(hComIn,console_mode);
		hComOut = GetStdHandle(STD_OUTPUT_HANDLE);
		break;
	}
}

// Follows are some routines to support legacy console operations

// typedef struct {
//  COORD      dwSize;              Size of screen buffer 
//  COORD      dwCursorPosition;    Location of cusor in buffer
//  WORD       wAttributes;         character attributes
//  SMALL_RECT srWindow;            coordinates of display window
//  COORD      dwMaximumWindowSize; maximum window size
// } CONSOLE_SCREEN_BUFFER_INFO;

// scroll screen up or down the console buffer
void scroll_console(int lines) {
	SMALL_RECT rect;
	COORD loc;
	CHAR_INFO fill;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	rect.Left = 0;
	rect.Top = 0;
	rect.Right = csbi.dwSize.X;
	rect.Bottom = csbi.dwSize.Y;
	fill.Char.UnicodeChar = TEXT(' ');
    fill.Attributes = csbi.wAttributes;
	loc.X = 0;
	loc.Y = (SHORT) lines;
	ScrollConsoleScreenBuffer(hComOut,&rect,NULL,loc,&fill);
}

// Move cursor to x, y location 
void move_console_cursor(int x, int y) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	// screen should be scrolled to keep cursor in window
	if ((x >= 0) && (x <= csbi.dwSize.X)) csbi.dwCursorPosition.X = x;
	if ((y >= 0) && (y <= csbi.dwSize.Y)) csbi.dwCursorPosition.Y = y;
	SetConsoleCursorPosition(hComOut, csbi.dwCursorPosition);
}

// Move cursor right
void moveright() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	if ( csbi.dwCursorPosition.X < csbi.dwSize.X) {
		csbi.dwCursorPosition.X += 1;
		SetConsoleCursorPosition(hComOut, csbi.dwCursorPosition);
	} 
}

// Move cursor left 
void moveleft() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	if ( csbi.dwCursorPosition.X > 0 ) {
		csbi.dwCursorPosition.X -= 1;
		SetConsoleCursorPosition(hComOut, csbi.dwCursorPosition);
	} 
}

void clearscreen()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	scroll_console(-csbi.dwSize.Y);
//	cursorhome();
	move_console_cursor(0,0);
//	SetConsoleCursorPosition(0,0);
}

// Move cursor down 
void movedown() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);

	if ( csbi.dwCursorPosition.Y < csbi.srWindow.Bottom ) {
		csbi.dwCursorPosition.Y += 1;
		SetConsoleCursorPosition(hComOut, csbi.dwCursorPosition);
	} else {
		scroll_console(-1);
	}
}

// Move cursor up 
void moveup() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	if ( csbi.dwCursorPosition.Y == 0 ) return;
	if (csbi.dwCursorPosition.Y > csbi.srWindow.Top) {
		csbi.dwCursorPosition.Y -= 1;
		SetConsoleCursorPosition(hComOut, csbi.dwCursorPosition);
	} else {
		scroll_console(1);
	}
}

void console_cleol() {
	DWORD cnt, tmp;
    COORD loc;
	TCHAR fill = TEXT(' ');
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
    loc.X = csbi.dwCursorPosition.X;
    loc.Y = csbi.dwCursorPosition.Y;
	cnt = csbi.dwSize.X - loc.X;
	if (cnt > 0) {
		FillConsoleOutputCharacter(hComOut,fill,cnt,loc,&tmp);
	}
}

void console_cleob() {
	DWORD val;
    COORD loc;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
    loc.X = csbi.dwCursorPosition.X;
    loc.Y = csbi.dwCursorPosition.Y;
	DWORD cnt = (csbi.dwSize.X - loc.X) + 
				(csbi.dwSize.Y - loc.Y) * csbi.dwSize.X;
	TCHAR fill = TEXT(' ');
	if (cnt > 0) {
		FillConsoleOutputCharacter(hComOut,fill,cnt,loc,&val);
	}
}

void creturn() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	csbi.dwCursorPosition.X = 0;
	SetConsoleCursorPosition(hComOut, csbi.dwCursorPosition);
}

void console_eraseline() {
	creturn();
	console_cleol();
}

int trans_console_color(int os9_color) {
//  {white,blue,black,green,red,yellow.magenta,cyan}
	int tbl[8] = {0xF,0x1,0x0,0X2,0x4,0xE,0xD,0xB};
	return tbl[os9_color & 7];
}

void console_background(int os9_color) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	WORD forground = csbi.wAttributes & 0xFF0F;
	WORD background = trans_console_color(os9_color) << 4;
	WORD color = forground + background;
    SetConsoleTextAttribute ( hComOut, color );
}

void console_forground(int os9_color) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hComOut, &csbi);
	WORD background = csbi.wAttributes & 0xFFF0;
	WORD forground = trans_console_color(os9_color);
	WORD color = forground + background;
    SetConsoleTextAttribute ( hComOut, color );
}

// Output sequence types 
#define SEQ_GOTOXY   1
#define SEQ_COMMAND  2
#define SEQ_BCOLOR   3
#define SEQ_BOLDSW   4
#define SEQ_BORDER   5
#define SEQ_DWEND    6
#define SEQ_FCOLOR   7
#define SEQ_SELECT   8
#define SEQ_CURSOR   9
#define SEQ_CONTROL 10
        
int  SeqType = 0;
int  SeqArgsNeeded = 0;
int  SeqArgsCount = 0;
char SeqArgs[8];

// write com
void acia_write_com() {
	DWORD tmp;
	char cc[16]; // for logging

	// Do we need one or more bytes to finish a command?
	if (SeqArgsNeeded) {

		SeqArgs[SeqArgsCount++] = Output_chr;
	   	if (SeqArgsCount < SeqArgsNeeded) return;

		// Once here sequence is complete
		SeqArgsNeeded = 0;
	    SeqArgsCount = 0;

		// Process the sequence
		switch (SeqType) {
		case SEQ_CONTROL:
			switch (SeqArgs[0]) {
			case 0x20: // Reverse video on
			case 0x21: // Reverse video off
			case 0x22: // Underline on
			case 0x23: // Underline off
			case 0x24: // Blinking on
			case 0x25: // Blinking off
//			case 0x30: // Insert line
//			case 0x31: // Delete line
				break;
			default:
				sprintf(cc,"~1F%02X~",SeqArgs[0]);     // not handled
				WriteConsole(hComOut,cc,6,&tmp,NULL);
			}
			break;
		case SEQ_GOTOXY:
			// row,col bias is 32
        	move_console_cursor(SeqArgs[0]-32, SeqArgs[1]-32);
			break;
		case SEQ_COMMAND:
			// Arg is another sequence
			switch (SeqArgs[0]) {
			case 0x32:
				SeqType = SEQ_FCOLOR;
			   	SeqArgsNeeded = 1;
				break;
			case 0x33:
				SeqType = SEQ_BCOLOR;
				SeqArgsNeeded = 1;
				break;
			case 0x34:
				SeqType = SEQ_BORDER;
				SeqArgsNeeded = 1;
				break;
			case 0x3d:
				SeqType = SEQ_BOLDSW;
				SeqArgsNeeded = 1;
				break;
			default:
				sprintf(cc,"~1B%02X~",SeqArgs[0]);     // not handled
				WriteConsole(hComOut,cc,strlen(cc),&tmp,NULL);
			}
			break;
		case SEQ_FCOLOR:
//			console_forground(Output_chr);
			break; 
		case SEQ_BCOLOR:
//			console_background(Output_chr);
			break; 
		case SEQ_BORDER:
			case SEQ_BOLDSW:
			break; 
		}
		return;
	}

	// write printable chr to console
	if  (isprint(Output_chr)) {
		WriteConsole(hComOut,&Output_chr,1,&tmp,NULL);
		return;
	}

	// process non printable
	switch (Output_chr) {
	case 0x0:
		break;
	case 0x1:        // cursor home
    	move_console_cursor(0,0);
		break;
	case 0x2:
		SeqType = SEQ_GOTOXY;
		SeqArgsNeeded = 2;
		break;
	case 0x3:        // clear line
		console_eraseline();
		break;
	case 0x4:        // clear to end of line
		console_cleol();
		break;
	case 0x5:
		SeqType = SEQ_CURSOR;
	    SeqArgsNeeded = 1;
		break;
	case 0x6:        // vt
		moveright();
		break;
	case 0x7:        // bell 
		WriteConsole(hComOut,&Output_chr,1,&tmp,NULL);
		break;
	case 0x8:        // backspace
		moveleft();
		break;
	case 0x9:        // up arrow
		moveup();
		break;
	case 0xA:        // line feed
		movedown();
		break;
	case 0xB:        // clear to end of screen
		console_cleob();
		break;
	case 0xC:        // clear screen 
		clearscreen();
		break;
	case 0xD:        // carriage return
		creturn();
		break;			
	case 0x1B:		 // Screen command
		SeqType = SEQ_COMMAND;
	    SeqArgsNeeded = 1;
		break;
	case 0x1F:       // Text control
		SeqType = SEQ_CONTROL;
	    SeqArgsNeeded = 1;
		break;
	default:         // unhandled
		sprintf(cc,"~%02X~",Output_chr);
		WriteConsole(hComOut,cc,4,&tmp,NULL);
	}
	return;
}

// read com
void acia_read_com() {
	DWORD val = 0;
	switch (comtype) {

	case 0: // Console
		Input_chr = _getch();
		if ((Input_chr == 0x00) || (Input_chr == 0xE0)) {
			Input_chr = _getch();
			switch(Input_chr) {
			case 0x48:  //up
				Input_chr = 0x0C;
				break;
			case 0x50:  //down
				Input_chr = 0x0A;
				break;
			case 0x4B:  //left
				Input_chr = 0x08;
				break;
			case 0x4D:  //right
				Input_chr = 0x09;
				break;
			case 0x73: 
				Input_chr = 0x10; //control left
				break;
			case 0x74: 
				Input_chr = 0x11; //control left 
				break;
			case 0x9B: //alt left (no shift left on win console)
				Input_chr = 0x18; //shift left
				break;
			case 0x9D: //alt right
				Input_chr = 0x19; //shift right
				break;
			}
		}
		break;
	}
	return;
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
			data = Input_chr;
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
		    Output_chr = data;
			acia_write_com();
//logOutChr();
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
