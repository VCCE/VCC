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

//------------------------------------------------------------------
// Console I/O
//------------------------------------------------------------------

#include "acia.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// Macros for key state modifiers
#define modshift(state)  (state & SHIFT_PRESSED)
#define modctrl(state)   (state & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
#define modalt(state)    (state & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
#define modextend(state) (state & ENHANCED_KEY)

//------------------------------------------------------------------
// private functions
//------------------------------------------------------------------
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
void console_background(int);
void console_forground(int);

//------------------------------------------------------------------
// Globals
//------------------------------------------------------------------
HANDLE hConIn = nullptr;               // Com input handle
HANDLE hConOut = nullptr;              // Com output handle
CONSOLE_SCREEN_BUFFER_INFO Csbi;    // Console buffer info

INPUT_RECORD KeyEvents[128];        // Buffer for keyboard records
INPUT_RECORD *EventsPtr=KeyEvents;  // Buffer pointer
int Event_Cnt = 0;                  // Buffer count

// Table to translate CoCo color to console color
//  {white,blue,black,green,red,yellow,magenta,cyan}
int color_tbl[8] = {0xF,0x1,0x0,0X2,0xC,0xE,0xD,0xB};

//----------------------------------------------------------------
// Open Console
//----------------------------------------------------------------

int
console_open() {
        DWORD mode;

//      Make sure console is closed first
        if ( hConOut != nullptr) console_close();
        if ( hConIn != nullptr) console_close();
        AllocConsole();

//      Disable the close button and "Close" context menu item of the
//      Console window to prevent inadvertant exit of the emulator
        HWND hwnd = GetConsoleWindow();
        HMENU hmenu = GetSystemMenu(hwnd, FALSE);
        EnableMenuItem(hmenu, SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);

        hConIn=GetStdHandle(STD_INPUT_HANDLE);

//      Allow quick edit and echo when in line mode
        mode = ENABLE_ECHO_INPUT |
               ENABLE_LINE_INPUT |
               ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hConIn,mode);

        FlushConsoleInputBuffer(hConIn);

//      Default to raw mode.
        AciaLineInput = 0;

        hConOut=GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTitle("VCC Console");

        return 0;
}

//------------------------------------------------------------------
// Close Console
//------------------------------------------------------------------

void console_close() {
    FreeConsole();
    hConOut = nullptr;
    hConIn = nullptr;
}

//----------------------------------------------------------------
// Read console.  Blocks until at least one char is read.
//----------------------------------------------------------------
int console_read(char * buf, int len) {

    char txt[64];  // For debug messages to win title

    int ascii; // Ascii char code (zero for control codes)
    int scode; // windows scan code for key press
    int state; // key state flags (modifiers, extended keys)

    int keypress_cnt = 0;
    unsigned char chr;

    // Abort if console not open or no space in buffer
    if ( hConIn == nullptr) return 0;
    if (len < 1) return 0;

    // If line mode return next line from keyboard buffer (blocks)
    if (AciaLineInput) {
        DWORD cnt = 0;
        ReadConsole(hConIn,buf,len,&cnt,nullptr);
        return cnt;
    }

    // If not line mode loop until at least one key press is found
    for(;;) {

        // If Event buffer is empty load it (blocks)
        if (Event_Cnt < 1) {
            EventsPtr = KeyEvents;
            DWORD cnt = 0;
            ReadConsoleInput(hConIn,KeyEvents,128,&cnt);
            Event_Cnt = cnt;
            if (Event_Cnt < 1) return 0;
        }

        // Process events in buffer. Only care about key down events.
        while (Event_Cnt--) {
            if (EventsPtr->Event.KeyEvent.bKeyDown) {
                ascii = EventsPtr->Event.KeyEvent.uChar.AsciiChar;
                scode = EventsPtr->Event.KeyEvent.wVirtualScanCode;
                state = EventsPtr->Event.KeyEvent.dwControlKeyState;

                // Standard Ascii character
                if (ascii) {
                    chr = ascii;
                    if (modalt(state)) {       // alt trans to graph chars
                        chr += 128;
                    } else if (chr == 0x1B) {  // Map escape to break (^E)
                        chr = 0x05;
                    } else if (chr == 0x1A) {  // Map ^Z to EOF (escape)
                        chr = 0x1B;
                    }
                    *buf++ = chr;
                    keypress_cnt++;

                // Function key.  Process per scancode.
                } else {

                    // Set highbit of scan code if extended key
                    if modextend(state) scode += 0x80;
                    switch(scode) {
                    case DIK_UP:
                    case DIK_NUMPAD8:
                        if (modshift(state))
                            *buf++ = 0x1C;        // shift up
                        else
                            *buf++ = 0x0C;        // up
                        keypress_cnt++;
                        break;
                    case DIK_DOWN:
                    case DIK_NUMPAD2:
                        if (modshift(state))
                            *buf++ = 0x1A;        // shift down
                        else
                            *buf++ = 0x0A;        // down
                        keypress_cnt++;
                        break;
                    case DIK_LEFT:
                    case DIK_NUMPAD4:
                        if (modshift(state))
                            *buf++ = 0x18;        // shift left
                        else if (modctrl(state))
                            *buf++ = 0x10;        // control left
                        else
                            *buf++ = 0x08;        // right
                        keypress_cnt++;
                        break;
                    case DIK_RIGHT:
                    case DIK_NUMPAD6:
                        if (modshift(state))
                            *buf++ = 0x19;        // shift right
                        else if (modctrl(state))
                            *buf++ = 0x11;        // control right
                        else
                            *buf++ = 0x09;        // right
                        keypress_cnt++;
                        break;
                    case DIK_F1:
                    case DIK_F2:
                    case DIK_F3:
                    case DIK_F4:
                    case DIK_F5:
                    case DIK_F6:
                    case DIK_F7:
                    case DIK_F8:
                    case DIK_F9:
                    case DIK_F10:
                    case DIK_F11:
                    case DIK_F12:
                sprintf(txt,"F-%x",scode);
                SetConsoleTitle(txt);
                        break;
                    }
                }
            }
            // Done with event
            EventsPtr++;
            // Return if input buffer full
            if (keypress_cnt == len) return keypress_cnt;
        }
        // Done processing events.
        // Return if at least one keystroke
        if (keypress_cnt > 0) return keypress_cnt;
    }
}

//----------------------------------------------------------------
// Write to Console
//----------------------------------------------------------------

// Terminal control sequence names
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

int console_write(const char *buf, int len) {

    int cnt = 0;
    int chr;

    DWORD tmp;
    char cc[80];

    // Abort if console not open
    if ( hConOut == nullptr) return 0;

    while (cnt < len) {
        chr = *buf++;
        cnt++;

        // Do we need to finish a command sequence?
        if (SeqArgsNeeded) {
            // Save the the chr as command arg
            SeqArgs[SeqArgsCount++] = chr;
    
            // If not done get more
            if (SeqArgsCount < SeqArgsNeeded) continue; // return cnt;

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
                case 0x40: // Line mode off
                    SetConsoleTitle("VCC Console");
                    AciaLineInput = 0;
                    break;
                case 0x41: // Line mode on
                    SetConsoleTitle("VCC Console Line Mode");
                    AciaLineInput = 1;
                    break;
                default:
                    sprintf(cc,"~1F%02X~",SeqArgs[0]);     // not handled
                    WriteConsole(hConOut,cc,6,&tmp,nullptr);
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
                    WriteConsole(hConOut,cc,strlen(cc),&tmp,nullptr);
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
            //return cnt;
            continue;
        }

        // write printable chr to console
        if  (isprint(chr)) {
            WriteConsole(hConOut,&chr,1,&tmp,nullptr);
            //return cnt;
            continue;
        }

        // write chars with high bit set to console (utf8)
        if (chr > 128) {
            WriteConsole(hConOut,&chr,1,&tmp,nullptr);
            continue;
            //return cnt;
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
            WriteConsole(hConOut,&chr,1,&tmp,nullptr);
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
        case 0xE:        // Unknown code
        case 0x15:       // Unknown code
            break;
        case 0x1B:       // Screen command
            SeqType = SEQ_COMMAND;
            SeqArgsNeeded = 1;
            break;
        case 0x1F:       // Text control
            SeqType = SEQ_CONTROL;
            SeqArgsNeeded = 1;
            break;
        default:         // unhandled
            sprintf(cc,"~%02X~",chr);
            WriteConsole(hConOut,cc,4,&tmp,nullptr);
        }
    }
    return cnt;
}

//------------------------------------------------------------------
// Console control functions
//------------------------------------------------------------------

// Scroll the screen buffer. Delete lines from top or bottom
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

// Insert line at cursor
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
    ScrollConsoleScreenBuffer(hConOut,&rect,nullptr,dest,&fill);
}

// Delete line at cursor
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
    ScrollConsoleScreenBuffer(hConOut,&rect,nullptr,dest,&fill);
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

// Erase line
void console_eraseline() {
    console_cr();
    console_cleol();
}

// Set background color
void console_background(int coco_color) {
    WORD color;
    WORD atrib;

    GetConsoleScreenBufferInfo(hConOut, &Csbi);
    atrib = Csbi.wAttributes & 0xFF0F; //mask out background

    if (coco_color > 7) // Background defaults to black
        color = 0;
    else
        color = color_tbl[coco_color];

    atrib |= ( color << 4 );
    SetConsoleTextAttribute ( hConOut, atrib );
}

// Set forground color
void console_forground(int coco_color) {
    WORD color;
    WORD atrib;

    GetConsoleScreenBufferInfo(hConOut, &Csbi);
    atrib = Csbi.wAttributes & 0xFFF0;  //mask out foreground

    if (coco_color > 7) // Background defaults to white
        color = 15;
    else
        color = color_tbl[coco_color];

    atrib |= color;
    SetConsoleTextAttribute ( hConOut, atrib );
}
