#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include "stdio.h"

#include <conio.h>
#include <dinput.h> // for scancodes (DIK_)
#include "resource.h"

#define IRQ 1

//----------------------------------------------------------------
// Function templates 
//----------------------------------------------------------------

// Acia
DWORD WINAPI acia_input_thread(LPVOID);
void acia_init();
unsigned char acia_read(unsigned char);
void acia_write(unsigned char,unsigned short);
void acia_open_com();
void acia_write_com(unsigned char);
unsigned char acia_read_com();
void acia_close_com();

// Console
void console_open();
void console_close();
unsigned char console_read();
void console_write(unsigned char);

void console_scroll(int);
void console_insertline();
void console_deleteline();
void console_move(int,int);
void console_right();
void console_left();
void console_down();
void console_up();
void console_cls();
void console_cleol();
void console_cleob();
void console_cr();
void console_eraseline();
int  console_trans_color(int);
void console_background(int);
void console_forground(int);

//------------------------------------------------------------------------
// DLL entry and exports 
//------------------------------------------------------------------------

#define MAX_LOADSTRING 200

// vcc stuff

typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);

static HINSTANCE g_hinstDLL=NULL;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);

void (*AssertInt)(unsigned char,unsigned char)=NULL;
void BuildDynaMenu(void);
void LoadConfig(void);
void SaveConfig(void);

BOOL APIENTRY DllMain( HINSTANCE  hinstDLL,
                       DWORD  reason,
                       LPVOID lpReserved )
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        acia_init();
    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}

__declspec(dllexport) void
ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
{
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);		
	strcpy(ModName,"ACIA");
	return ;
}

__declspec(dllexport) void ModuleConfig(unsigned char MenuID)
{
//		DialogBox(g_hinstDLL, (LPCTSTR)IDD_PROPPAGE, NULL, (DLGPROC)Config);
//		BuildDynaMenu();
		return;
}

// Export write to port
__declspec(dllexport) void
PackPortWrite(unsigned char Port,unsigned char Data)
{
    acia_write(Data,Port);
    return;
}

// Export read from port
__declspec(dllexport) unsigned char 
PackPortRead(unsigned char Port)
{
    return acia_read(Port);
}

// This captures the transfer point for the CPU assert interupt 
__declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
{
	AssertInt=Dummy;
	return;
}

//------------------------------------------------------------------------
// ACIA
//------------------------------------------------------------------------

//----------------------------------------------------------------
// Acia Globals
//----------------------------------------------------------------

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

HANDLE hEvents[2];
enum {
    EVT_DAT_REG_READ,      // Data register has been read 
    EVT_CMD_REG_WRITE      // Command register was written
};

int comtype = 0;           // Default 0 = console
int	acia_initialized = 0;

//------------------------------------------------------------------------
//  Initiallize acia 
//------------------------------------------------------------------------

void acia_init()
{
    DWORD id;
	if (acia_initialized == 0) {
	    acia_open_com();
		CreateThread(NULL,0,acia_input_thread,NULL,0,&id);
		hEvents[1] = CreateEvent (NULL,FALSE,FALSE,NULL);
		hEvents[0] = CreateEvent (NULL,FALSE,FALSE,NULL);
		acia_initialized = 1;
	}
}

//------------------------------------------------------------------------
// Input Thread.
//------------------------------------------------------------------------

// Reads hang. They are done in a seperate thread.
DWORD WINAPI acia_input_thread(LPVOID param) 
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
// ACIA Port I/O
//------------------------------------------------------------------------

unsigned char acia_read(unsigned char port)
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

//------------------------------------------------------------------------
// ACIA Port I/O
//------------------------------------------------------------------------

void acia_write(unsigned char data,unsigned short port)
{
//	acia_init(); // Until we have some other way to init

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
		    if (CmdReg & CmdDTR) {
			    StatReg = StatTxE;                     // mark i/o buffers empty
				SetEvent(hEvents[EVT_CMD_REG_WRITE]);  // Tell input worker
			} else {
				StatReg = 0;
			}
			break;
		case 0x6B:
			CtlReg = data;  // Not used, just returned on read
			break;
    }
}

//----------------------------------------------------------------
// Acia com hooks. These dispatch to communication type used
//----------------------------------------------------------------

// Open com
void acia_open_com() {
	switch (comtype) {
	case 0: // Legacy Console
        console_open();
		break;
	}
}

void acia_write_com(unsigned char chr) {
	switch (comtype) {
	case 0: // Legacy Console
        console_write(chr);
        break;
    }
}

unsigned char acia_read_com() {
	switch (comtype) {
	case 0: // Legacy Console
        return console_read();
    }
    return 0;
}

void acia_close_com() {
	switch (comtype) {
	case 0: // Console
        console_close();
		break;
	}
}

//------------------------------------------------------------------
// Console I/O and management
//------------------------------------------------------------------

// Globals
HANDLE hConIn;                    // Com input handle
HANDLE hConOut;                   // Com output handle
CONSOLE_SCREEN_BUFFER_INFO Csbi;  // Console buffer info

void putdline(int x, char *str);  // Put debug text at top of console

//  CONSOLE_SCREEN_BUFFER_INFO:
//    COORD      dwSize              Size of screen buffer 
//    COORD      dwCursorPosition    Location of cusor in buffer
//    WORD       wAttributes         character attributes
//    SMALL_RECT srWindow            coordinates of display window
//    COORD      dwMaximumWindowSize maximum window size

void
console_open() {
		AllocConsole();
		hConIn=GetStdHandle(STD_INPUT_HANDLE);
//      Disable as much standard input console processing as possible
	    SetConsoleMode(hConIn,0);
		FlushConsoleInputBuffer(hConIn);
		hConOut=GetStdHandle(STD_OUTPUT_HANDLE);
        COORD bufsiz={80,250};
        SetConsoleScreenBufferSize(hConOut,bufsiz);
        SMALL_RECT winsiz = {0,0,80,24};
        SetConsoleWindowInfo(hConOut, TRUE, &winsiz);
}

//----------------------------------------------------------------
// Console output and control
//----------------------------------------------------------------

// Terminal control sequence names for clairty 
enum {
    SEQ_NONE,
    SEQ_GOTOXY,   // Position cursor to X,Y
    SEQ_COMMAND,  // A command sequence
    SEQ_CONTROL,  // A control sequence
    SEQ_BCOLOR,   // Set background color
    SEQ_BOLDSW,   // Set character attrib (TODO)
    SEQ_BORDER,   // Set border color
    SEQ_FCOLOR,   // Set forground color
    SEQ_CURSOR    // Cursor on/off (TODO)
};
int  SeqType = SEQ_NONE;
int  SeqArgsNeeded = 0;
int  SeqArgsCount = 0;
char SeqArgs[8];

void console_write(unsigned char chr) {
	DWORD tmp;
	char cc[16];  // for logging

	// Do we need to finish a command?
	if (SeqArgsNeeded) {
        // Save the the chr as command arg
		SeqArgs[SeqArgsCount++] = chr;

	    // If not done wait for more
	   	if (SeqArgsCount < SeqArgsNeeded) return;

        // Sequence complete
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
                break;
			case 0x30: // Insert line
                console_insertline();
                break;
			case 0x31: // Delete line
                console_deleteline(); 
				break;
			default:
				sprintf(cc,"~1F%02X~",SeqArgs[0]);     // not handled
				WriteConsole(hConOut,cc,6,&tmp,NULL);
			}
			break;
		case SEQ_GOTOXY:
			// row,col bias is 32
        	console_move(SeqArgs[0]-32, SeqArgs[1]-32);
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
				WriteConsole(hConOut,cc,strlen(cc),&tmp,NULL);
			}
			break;
		case SEQ_FCOLOR:
			console_forground(chr);
			break; 
		case SEQ_BCOLOR:
			console_background(chr);
			break; 
		case SEQ_BORDER:
		case SEQ_BOLDSW:
			break; 
		}
		return;
	}

	// write printable chr to console
	if  (isprint(chr)) {
		WriteConsole(hConOut,&chr,1,&tmp,NULL);
		return;
	}

    // write chars with high bit set to console (utf8)
    if (chr > 128) {
		WriteConsole(hConOut,&chr,1,&tmp,NULL);
		return;
    }

	// process non printable
	switch (chr) {
	case 0x0:
		break;
	case 0x1:        // cursor home
    	console_move(0,0);
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
		console_right();
		break;
	case 0x7:        // bell 
		WriteConsole(hConOut,&chr,1,&tmp,NULL);
		break;
	case 0x8:        // backspace
		console_left();
		break;
	case 0x9:        // up arrow
		console_up();
		break;
	case 0xA:        // line feed
		console_down();
		break;
	case 0xB:        // clear to end of screen
		console_cleob();
		break;
	case 0xC:        // clear screen 
		console_cls();
		break;
	case 0xD:        // carriage return
		console_cr();
		break;			
	case 0xE:        // Unknown code gen by basic09
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
		sprintf(cc,"~%02X~",chr);
		WriteConsole(hConOut,cc,4,&tmp,NULL);
	}
	return;
}

//----------------------------------------------------------------
// Console Input
//----------------------------------------------------------------

// Structure for console keypress data
typedef struct _KeyData {
    int ascii; // Ascii char code (zero for control codes)
    int scode; // windows scan code
    int state; // RIGHT_ALT_PRESSED  0x0001
               // LEFT_ALT_PRESSED   0x0002
               // RIGHT_CTRL_PRESSED 0x0004
               // LEFT_CTRL_PRESSED  0x0008
               // SHIFT_PRESSED      0x0010
               // NUMLOCK_ON         0x0040
               // ENHANCED_KEY       0x0100 (not a numpad key)
} KeyData;

INPUT_RECORD ConsoleEvents[16]={0};           // Buffer for console event records
INPUT_RECORD *ConsoleEventsPtr=ConsoleEvents; // Input record buffer pointer 

// Get win events from console. Returns with keypress data
// Data for each key press is returned in KeyData structure
BOOL
GetConsoleKeyPress(KeyData *kdat)
{
    DWORD cnt;
    while(TRUE) {
        ConsoleEventsPtr++;
        switch (ConsoleEventsPtr->EventType) { 
        case 0:
            ReadConsoleInput(hConIn,ConsoleEvents,16,&cnt);
            if (cnt == 0) return FALSE;
            ConsoleEvents[cnt].EventType=0;
            ConsoleEventsPtr = ConsoleEvents-1;
            break;
        case KEY_EVENT:
            if (ConsoleEventsPtr->Event.KeyEvent.bKeyDown) {
                kdat->ascii = ConsoleEventsPtr->Event.KeyEvent.uChar.AsciiChar;
                kdat->scode = ConsoleEventsPtr->Event.KeyEvent.wVirtualScanCode;
                kdat->state = ConsoleEventsPtr->Event.KeyEvent.dwControlKeyState;
                return TRUE;
            }
            break;
        case WINDOW_BUFFER_SIZE_EVENT:
            break;
            // TODO: handle resize event
        }
    }
}

//----------------------------------------------------------------
// Read console key, map it to coco input character, and return it
// Blocks until a valid input character is read from console
//----------------------------------------------------------------

#define modshift(state) (state & SHIFT_PRESSED)
#define modctrl(state) (state & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
#define modalt(state) (state & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
#define modextend(state) (state & ENHANCED_KEY)

unsigned char 
console_read() {
    KeyData key;

    while (GetConsoleKeyPress(&key) ) {

        // Non function keys
        if (key.ascii) {
            unsigned char chr = key.ascii;
            if (modalt(key.state)) {   // alt trans to graph chars
                chr += 128;
            } else if (chr == 0x1B) {  // Map escape to break (^E)
                chr = 0x05;
            } else if (chr == 0x1A) {  // Map ^Z to EOF (escape)
                chr = 0x1B;
            }
            return chr;
        }

        // Function keys have no ascii equivalent
        int sc = key.scode;
        if modextend(key.state) sc += 0x80;

	    switch(sc) {
        case DIK_UP:
	    case DIK_NUMPAD8:
            if (modshift(key.state))
                return 0x1C;        // shift up
            else 
                return 0x0C;        // up
        case DIK_DOWN:
	    case DIK_NUMPAD2:
            if (modshift(key.state))
		        return 0x1A;        // shift down
            else
		        return 0x0A;        // down
        case DIK_LEFT:
	    case DIK_NUMPAD4:
            if (modshift(key.state))
		        return 0x18;        // shift left
            else if (modctrl(key.state))
			    return 0x10;        // control left
            else 
			    return 0x08;        // right
        case DIK_RIGHT:
	    case DIK_NUMPAD6:
            if (modshift(key.state))
			    return 0x19;        // shift right
            else if (modctrl(key.state))
			    return 0x11;        // control right 
            else 
			    return 0x09;        // right
        }
    }
    return 0;  // not reached
}

//------------------------------------------------------------------
// Close
//------------------------------------------------------------------

void console_close() {
    FreeConsole();
}

//------------------------------------------------------------------
// Console control functions
//------------------------------------------------------------------

// Put debug string at specified column on top line of screen.
void putdline(int x, char *str)
{
    DWORD tmp;
    COORD pos={0,0};
	GetConsoleScreenBufferInfo(hConOut, &Csbi);
    pos.X = x;
    pos.Y = Csbi.srWindow.Top;
    SetConsoleCursorPosition(hConOut,pos);
    WriteConsole(hConOut,str,strlen(str),&tmp,NULL);
    SetConsoleCursorPosition(hConOut,Csbi.dwCursorPosition);
}

// Scroll the screen buffer. Deletes lines from top or bottom
// of the buffer depending on the sign of the lines parameter.
// Negative number deletes lines from the top.
void console_scroll(int lines) {
	SMALL_RECT rect;
	SMALL_RECT clip;
	COORD loc;
	CHAR_INFO fill;
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	rect.Left = 0;
	rect.Top = 0;
	rect.Right = Csbi.dwSize.X;
	rect.Bottom = Csbi.dwSize.Y;
    clip = rect;
	fill.Char.UnicodeChar = TEXT(' ');
    fill.Attributes = Csbi.wAttributes;
	loc.X = 0;
	loc.Y = (SHORT) lines;
	ScrollConsoleScreenBuffer(hConOut,&rect,&clip,loc,&fill);
}

// Insert line where cursor is
void console_insertline() {
	SMALL_RECT rect;
    COORD dest;
    CHAR_INFO fill;
	GetConsoleScreenBufferInfo(hConOut, &Csbi);

    fill.Char.UnicodeChar = TEXT(' ');
    fill.Attributes = Csbi.wAttributes;
    rect.Left = 0;
    dest.X = 0;
    rect.Right = Csbi.dwSize.X;
    rect.Top = Csbi.dwCursorPosition.Y;
    rect.Bottom = Csbi.dwSize.Y-1;
    dest.Y = Csbi.dwCursorPosition.Y+1;
    ScrollConsoleScreenBuffer(hConOut,&rect,NULL,dest,&fill);
}

// Delete line where cursor is
void console_deleteline() {
	SMALL_RECT rect;
    COORD dest;
    CHAR_INFO fill;
	GetConsoleScreenBufferInfo(hConOut, &Csbi);

    fill.Char.UnicodeChar = TEXT(' ');
    fill.Attributes = Csbi.wAttributes;
    rect.Left = 0;
    dest.X = 0;
    rect.Right = Csbi.dwSize.X;

    rect.Top = Csbi.dwCursorPosition.Y+1;
    rect.Bottom = Csbi.dwSize.Y;
    dest.Y = Csbi.dwCursorPosition.Y;
    ScrollConsoleScreenBuffer(hConOut,&rect,NULL,dest,&fill);
}

// Move cursor to x, y location 
void console_move(int x, int y) {
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	if ((x >= 0) && (x <= Csbi.dwSize.X)) Csbi.dwCursorPosition.X = x;
	if ((y >= 0) && (y <= Csbi.dwSize.Y)) Csbi.dwCursorPosition.Y = y;
	SetConsoleCursorPosition(hConOut, Csbi.dwCursorPosition);
}

// Move cursor right
void console_right() {
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	Csbi.dwCursorPosition.X += 1;
	SetConsoleCursorPosition(hConOut, Csbi.dwCursorPosition);
}

// Move cursor left 
void console_left() {
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	Csbi.dwCursorPosition.X -= 1;
	SetConsoleCursorPosition(hConOut, Csbi.dwCursorPosition);
}

// Move cursor down 
void console_down() {
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	if ((Csbi.dwCursorPosition.Y+1) >= Csbi.dwSize.Y) {
        console_scroll(-1);
    } else {
        Csbi.dwCursorPosition.Y += 1;
	    SetConsoleCursorPosition(hConOut, Csbi.dwCursorPosition);
	}
}

// Move cursor up 
void console_up() {
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	if ( Csbi.dwCursorPosition.Y == 0 ) return;
	Csbi.dwCursorPosition.Y -= 1;
	SetConsoleCursorPosition(hConOut, Csbi.dwCursorPosition);
}

// Clear console buffer and home cursor
void console_cls()
{
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	console_scroll(-Csbi.dwSize.Y);
	console_move(0,0);
}

// Clear to end of line
void console_cleol() {
	DWORD cnt, tmp;
    COORD loc;
	TCHAR fill = TEXT(' ');
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
    loc.X = Csbi.dwCursorPosition.X;
    loc.Y = Csbi.dwCursorPosition.Y;
	cnt = Csbi.dwSize.X - loc.X;
	if (cnt > 0) {
		FillConsoleOutputCharacter(hConOut,fill,cnt,loc,&tmp);
	}
}

// Clear to end of buffer
void console_cleob() {
	DWORD val;
    COORD loc;
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
    loc.X = Csbi.dwCursorPosition.X;
    loc.Y = Csbi.dwCursorPosition.Y;
	DWORD cnt = (Csbi.dwSize.X - loc.X) + 
				(Csbi.dwSize.Y - loc.Y) * Csbi.dwSize.X;
	TCHAR fill = TEXT(' ');
	if (cnt > 0) {
		FillConsoleOutputCharacter(hConOut,fill,cnt,loc,&val);
	}
}

// Carriage return (move cursor to first column)
void console_cr() {
    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	Csbi.dwCursorPosition.X = 0;
	SetConsoleCursorPosition(hConOut, Csbi.dwCursorPosition);
}

// Erase entire line
void console_eraseline() {
	console_cr();
	console_cleol();
}

// Translate CoCo color to console color
//  {white,blue,black,green,red,yellow.magenta,cyan}
int console_trans_color(int os9_color) {
	int tbl[8] = {0xF,0x1,0x0,0X2,0x4,0xE,0xD,0xB};
	return tbl[os9_color];
}

// Set background color
void console_background(int os9_color) {
    WORD color;
    WORD atrib;

    GetConsoleScreenBufferInfo(hConOut, &Csbi);
    atrib = Csbi.wAttributes & 0xFF0F; //mask out background

    if (os9_color > 7) color = 0; // defaults to black
    else color = console_trans_color(os9_color);

    atrib |= ( color << 4 );
    SetConsoleTextAttribute ( hConOut, atrib );
}

// Set forground color
void console_forground(int os9_color) {
    WORD color;
    WORD atrib;

    GetConsoleScreenBufferInfo(hConOut, &Csbi);
	atrib = Csbi.wAttributes & 0xFFF0;  //mask out foreground

    if (os9_color > 7) color = 15; // defaults to white
    else color = console_trans_color(os9_color);

    atrib |= color;
    SetConsoleTextAttribute ( hConOut, atrib );
}

