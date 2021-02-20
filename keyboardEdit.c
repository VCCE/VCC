/*****************************************************************************/
/*
Vcc Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer)

	VCC (Virtual Color Computer) is free software: you can redistribute 
	it and/or modify it under the terms of the GNU General Public License 
	as published by the Free Software Foundation, either version 3 of the 
	License, or (at your option) any later version.

	VCC (Virtual Color Computer) is distributed in the hope that it will 
	be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
	of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with VCC (Virtual Color Computer).  If not, see 
	<http://www.gnu.org/licenses/>.

Author of this file: E J Jaquay 2021

*/

#include "keyboard.h"
#include "keyboardLayout.h"
#include "keyboardEdit.h"

//#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <windowsx.h>
//#include <dinput.h>

//#include "resource.h"
#include <stdio.h>

#include "logger.h"

/******************************************************************** 
 * ScanCode table associates PC scan code text with scan code values
 *
 * ScanCodes are from dinput.h
 *
 * This table must be sorted alphabetically by keyname!
 *
 * DIK_ is removed from keyname strings for brevity
 * If Key label strings are NULL the keyname is used as label
 ********************************************************************/

struct PCScanCode {
    char* keyname;           // Keyname in custom keymap file
    char* label;             // Label for keymap editor display
    unsigned char ScanCode;	 // Scan code from windows kb event
};

static struct PCScanCode sctable[] =
{
    {"0",              NULL,         DIK_0           },
    {"1",              NULL,         DIK_1           },
    {"2",              NULL,         DIK_2           },
    {"3",              NULL,         DIK_3           },
    {"4",              NULL,         DIK_4           },
    {"5",              NULL,         DIK_5           },
    {"6",              NULL,         DIK_6           },
    {"7",              NULL,         DIK_7           },
    {"8",              NULL,         DIK_8           },
    {"9",              NULL,         DIK_9           },
    {"A",              NULL,         DIK_A           },
    {"ADD",            "Add",        DIK_ADD         },
    {"APOSTROPHE",     "Apostrophe", DIK_APOSTROPHE  },
    {"AT",             "@",          DIK_AT          },
    {"B",              NULL,         DIK_B           },
    {"BACK",           "Backspace",  DIK_BACK        },
    {"BACKSLASH",      "BackSlash",  DIK_BACKSLASH   },
    {"BACKSPACE",      "BackSpace",  DIK_BACKSPACE   },
    {"C",              NULL,         DIK_C           },
    {"CAPSLOCK",       "Caplock",    DIK_CAPSLOCK    },
    {"COLON",          "Colon",      DIK_COLON       },
    {"COMMA",          "Comma",      DIK_COMMA       },
    {"D",              NULL,         DIK_D           },
    {"DECIMAL",        "Decimal",    DIK_DECIMAL     },
    {"DELETE",         "Delete",     DIK_DELETE      },
    {"DIVIDE",         "Divide",     DIK_DIVIDE      },
    {"DOWN",           "Down",       DIK_DOWN        },
    {"DOWNARROW",      NULL,         DIK_DOWNARROW   },
    {"E",              NULL,         DIK_E           },
    {"END",            "End",        DIK_END         },
    {"EQUALS",         "Equals",     DIK_EQUALS      },
    {"ESCAPE",         "Esc",        DIK_ESCAPE      },
    {"F",              NULL,         DIK_F           },
    {"F1",             NULL,         DIK_F1          },
    {"F10",            NULL,         DIK_F10         },
    {"F11",            NULL,         DIK_F11         },
    {"F12",            NULL,         DIK_F12         },
    {"F13",            NULL,         DIK_F13         },
    {"F14",            NULL,         DIK_F14         },
    {"F15",            NULL,         DIK_F15         },
    {"F2",             NULL,         DIK_F2          },
    {"F3",             NULL,         DIK_F3          },
    {"F4",             NULL,         DIK_F4          },
    {"F5",             NULL,         DIK_F5          },
    {"F6",             NULL,         DIK_F6          },
    {"F7",             NULL,         DIK_F7          },
    {"F8",             NULL,         DIK_F8          },
    {"F9",             NULL,         DIK_F9          },
    {"G",              NULL,         DIK_G           },
    {"GRAVE",          "Grave",      DIK_GRAVE       },
    {"H",              NULL,         DIK_H           },
    {"HOME",           "Home",       DIK_HOME        },
    {"I",              NULL,         DIK_I           },
    {"INSERT",         "Insert",     DIK_INSERT      },
    {"J",              NULL,         DIK_J           },
    {"K",              NULL,         DIK_K           },
    {"L",              NULL,         DIK_L           },
    {"LALT",           "Alt",        DIK_LALT        },
    {"LBRACKET",       "LBracket",   DIK_LBRACKET    },
    {"LCONTROL",       "Ctrl",       DIK_LCONTROL    },
    {"LEFT",           "Left",       DIK_LEFT        },
    {"LEFTARROW",      NULL,         DIK_LEFTARROW   },
    {"LMENU",          "Alt",        DIK_LMENU       },
    {"LSHIFT",         "Shift",      DIK_LSHIFT      },
    {"LWIN",           "LWin",       DIK_LWIN        },
    {"M",              NULL,         DIK_M           },
    {"MINUS",          "Minus",      DIK_MINUS       },
    {"MULTIPLY",       "Mult",       DIK_MULTIPLY    },
    {"N",              NULL,         DIK_N           },
    {"NEXT",           "PgDn",       DIK_NEXT        },
    {"NUMLOCK",        "NumLock",    DIK_NUMLOCK     },
    {"NUMPAD0",        "NumPad0",    DIK_NUMPAD0     },
    {"NUMPAD1",        "NumPad1",    DIK_NUMPAD1     },
    {"NUMPAD2",        "NumPad2",    DIK_NUMPAD2     },
    {"NUMPAD3",        "NumPad3",    DIK_NUMPAD3     },
    {"NUMPAD4",        "NumPad4",    DIK_NUMPAD4     },
    {"NUMPAD5",        "NumPad5",    DIK_NUMPAD5     },
    {"NUMPAD6",        "NumPad6",    DIK_NUMPAD6     },
    {"NUMPAD7",        "NumPad7",    DIK_NUMPAD7     },
    {"NUMPAD8",        "NumPad8",    DIK_NUMPAD8     },
    {"NUMPAD9",        "NumPad9",    DIK_NUMPAD9     },
    {"NUMPADCOMMA",    "NP-Comma",   DIK_NUMPADCOMMA },
    {"NUMPADENTER",    "NP-Enter",   DIK_NUMPADENTER },
    {"NUMPADEQUALS",   "NP-Equals",  DIK_NUMPADEQUALS}, 
    {"NUMPADMINUS",    "NP-Minus",   DIK_NUMPADMINUS }, 
    {"NUMPADPERIOD",   "NP-Period",  DIK_NUMPADPERIOD}, 
    {"NUMPADPLUS",     "NP-Plus",    DIK_NUMPADPLUS  }, 
    {"NUMPADSLASH",    "NP-Slash",   DIK_NUMPADSLASH }, 
    {"NUMPADSTAR",     "NP-Star",    DIK_NUMPADSTAR  }, 
    {"O",              NULL,         DIK_O           }, 
    {"P",              NULL,         DIK_P           }, 
    {"PERIOD",         "Period",     DIK_PERIOD      }, 
    {"PGDN",           "PgDn",       DIK_PGDN        },
    {"PGUP",           "PgUp",       DIK_PGUP        },
    {"PRIOR",          "PgUp",       DIK_PRIOR       },
    {"Q",              NULL,         DIK_Q           },
    {"R",              NULL,         DIK_R           },
    {"RALT",           "Alt",        DIK_RALT        },
    {"RBRACKET",       "RBracket",   DIK_RBRACKET    },
    {"RCONTROL",       "Ctrl",       DIK_RCONTROL    },
    {"RETURN",         "Enter",      DIK_RETURN      },
    {"RIGHT",          "Right",      DIK_RIGHT       },
    {"RIGHTARROW",     NULL,         DIK_RIGHTARROW  },
    {"RMENU",          "Alt",        DIK_RMENU       },
    {"RSHIFT",         "Shift",      DIK_RSHIFT      },
    {"RWIN",           "RWin",       DIK_RWIN        },
    {"S",              NULL,         DIK_S           },
    {"SCROLL",         "Scroll",     DIK_SCROLL      },
    {"SEMICOLON",      "Semicolon",  DIK_SEMICOLON   },
    {"SLASH",          "/",          DIK_SLASH       },
    {"SPACE",          "Space",      DIK_SPACE       },
    {"SUBTRACT",       "Subtact",    DIK_SUBTRACT    },
    {"SYSRQ",          "SysReq",     DIK_SYSRQ       },
    {"T",              NULL,         DIK_T           },
    {"TAB",            "Tab",        DIK_TAB         },
    {"U",              NULL,         DIK_U           },
    {"UNDERLINE",      "_",          DIK_UNDERLINE   },
    {"UP",             "Up",         DIK_UP          },
    {"UPARROW",        NULL,         DIK_UPARROW     },
    {"V",              NULL,         DIK_V           },  
    {"W",              NULL,         DIK_W           }, 
    {"X",              NULL,         DIK_X           },  
    {"Y",              NULL,         DIK_Y           }, 
    {"Z",              NULL,         DIK_Z           },
    {NULL,             NULL,         0               }  // table end
};

/************************************************************************ 
 * CoCoKey table used to convert CoCo key names or edit dialog key ids. 
 * into CoCO keyboard row mask, and column numbers (rollover table)
 * There is an entry for each physical key on the coco keyboard
 *
 * This table must be sorted alphabetically by keyname!
 *
 * COCO_ is removed from keyname strings for brevity
 * If Key label strings are NULL the keyname is used
 *
 ************************************************************************/
 
struct CoCoKey {
	char* keyname;      // Keyname in custom keymap file
    char* label;        // Label for keymap editor display
	int id;             // Button ID on key edit dialog
	unsigned char row;  // bit mask for row select
	unsigned char col;  // col 0-7
};
 
static struct CoCoKey cctable[] =
{
     { "0",         NULL,      IDC_KEYBTN_0,      16,0 },   //  0
     { "1",         NULL,      IDC_KEYBTN_1,      16,1 },   //  1      !
     { "2",         NULL,      IDC_KEYBTN_2,      16,2 },   //  2      "
     { "3",         NULL,      IDC_KEYBTN_3,      16,3 },   //  3      #
     { "4",         NULL,      IDC_KEYBTN_4,      16,4 },   //  4      $
     { "5",         NULL,      IDC_KEYBTN_5,      16,5 },   //  5      %
     { "6",         NULL,      IDC_KEYBTN_6,      16,6 },   //  6      &
     { "7",         NULL,      IDC_KEYBTN_7,      16,7 },   //  7      '
     { "8",         NULL,      IDC_KEYBTN_8,      32,0 },   //  8      (
     { "9",         NULL,      IDC_KEYBTN_9,      32,1 },   //  9      )
     { "A",         NULL,      IDC_KEYBTN_A,       1,1 },   //  a      A
     { "ALT",       NULL,      IDC_KEYBTN_ALT,    64,3 },
     { "AT",        "@",       IDC_KEYBTN_AT,      1,0 },   //  @      
     { "B",         NULL,      IDC_KEYBTN_B,       1,2 },   //  b      B
     { "BREAK",     "Break",   IDC_KEYBTN_BRK,    64,2 },   //  BREAK  ESCAPE
     { "C",         NULL,      IDC_KEYBTN_C,       1,3 },   //  c      C
     { "CLEAR",     "Clear",   IDC_KEYBTN_CLR,    64,1 },   //  CLEAR
     { "COLON",     "Colon",   IDC_KEYBTN_COLON,  32,2 },   //  :      *
     { "COMMA",     "Comma",   IDC_KEYBTN_COMMA,  32,4 },   //  ,      <
     { "CTRL",      NULL,      IDC_KEYBTN_CTRL,   64,4 },
     { "D",         NULL,      IDC_KEYBTN_D,       1,4 },   //  d      D
     { "DOWN",      "Down",    IDC_KEYBTN_DOWN,    8,4 },   //  DOWN
     { "E",         NULL,      IDC_KEYBTN_E,       1,5 },   //  e      E
     { "ENTER",     "Enter",   IDC_KEYBTN_ENTER,  64,0 },   //  ENTER
     { "F",         NULL,      IDC_KEYBTN_F,       1,6 },   //  f      F
     { "F1",        NULL,      IDC_KEYBTN_F1,     64,5 },   //  F1
     { "F2",        NULL,      IDC_KEYBTN_F2,     64,6 },   //  F2
     { "G",         NULL,      IDC_KEYBTN_G,       1,7 },   //  g      G
     { "H",         NULL,      IDC_KEYBTN_H,       2,0 },   //  h      H
     { "I",         NULL,      IDC_KEYBTN_I,       2,1 },   //  i      I
     { "J",         NULL,      IDC_KEYBTN_J,       2,2 },   //  j      J
     { "K",         NULL,      IDC_KEYBTN_K,       2,3 },   //  k      K
     { "L",         NULL,      IDC_KEYBTN_L,       2,4 },   //  l      L
     { "LEFT",      "Left",    IDC_KEYBTN_LEFT,    8,5 },   //  LEFT  
     { "M",         NULL,      IDC_KEYBTN_M,       2,5 },   //  m      M
     { "MINUS",     "Minus",   IDC_KEYBTN_MINUS,  32,5 },   //  -      =
     { "N",         NULL,      IDC_KEYBTN_N,       2,6 },   //  n      N
     { "O",         NULL,      IDC_KEYBTN_O,       2,7 },   //  o      O
     { "P",         NULL,      IDC_KEYBTN_P,       4,0 },   //  p      P
     { "PERIOD",    "Period",  IDC_KEYBTN_PERIOD, 32,6 },   //  .      >
     { "Q",         NULL,      IDC_KEYBTN_Q,       4,1 },   //  q      Q
     { "R",         NULL,      IDC_KEYBTN_R,       4,2 },   //  r      R
     { "RIGHT",     "Right",   IDC_KEYBTN_RIGHT,   8,6 },   //  RIGHT
     { "S",         NULL,      IDC_KEYBTN_S,       4,3 },   //  s      S
     { "SEMICOLON", "Semicolon",IDC_KEYBTN_SEMI,  32,3 },   //  ;      +
     { "SHIFT",     "Shift",   IDC_KEYBTN_LSHIFT, 64,7 },   //  also RSHIFT
     { "SLASH",     "Slash",   IDC_KEYBTN_SLASH,  32,7 },   //  /      ?
     { "SPACE",     "Space",   IDC_KEYBTN_SPACE,   8,7 },   //  SPACE
     { "T",         NULL,      IDC_KEYBTN_T,       4,4 },   //  t      T
     { "U",         NULL,      IDC_KEYBTN_U,       4,5 },   //  u      U
     { "UP",        NULL,      IDC_KEYBTN_UP,      8,3 },   //  UP
     { "V",         NULL,      IDC_KEYBTN_V,       4,6 },   //  v      V
     { "W",         NULL,      IDC_KEYBTN_W,       4,7 },   //  w      W
     { "X",         NULL,      IDC_KEYBTN_X,       8,0 },   //  x      X     
     { "Y",         NULL,      IDC_KEYBTN_Y,       8,1 },   //  y      Y
     { "Z",         NULL,      IDC_KEYBTN_Z,       8,2 },   //  z      Z
     { NULL,        NULL,      NULL,               0,0 }    //  table ends
};

// Windows handles
HWND  hKeyMapDlg;     // Key map dialog
HWND  hEdit_PC;       // Control to display PC Key.
HWND  hEdit_CC;       // Control to display selected CoCo Keys
HWND  hEdit_MapFile;  // Control to display current map file name

// Subclassed Edit_PC control processing
WNDPROC Edit_PCproc;
LRESULT CALLBACK SubEdit_PCproc(HWND,UINT,WPARAM,LPARAM);

// CoCo and Host PC buttons and keys that are currently selected
// Note Left and Right modifiers are tied together
int CC_KeySelected;  // BtnId
int CC_ModSelected;  // 0,1,2,3
int PC_KeySelected;  // ScanCode
int PC_ModSelected;  // 0,1,2,3
 
// Modifier Names for edit box display
static char *ModName[4] = {"","Shift","Ctrl","Alt"};

// Flag to limit the number of popup error messages
// when loading the custom keymap from a file
int UserWarned = 0;

// Current CoCo key button states (up/down)
BOOL CoCoKeySet;      // BtnId
BOOL CoCoModSet;      // 0,1,2,3

// Forward references
BOOL  InitKeymapDialog(HWND);
BOOL  Process_CoCoKey(int);
void  CoCoModifier(int);
void  ShowCoCoKey();
void  SetPCmod(int);
void  ShowPCkey();
void  DoKeyDown(WPARAM,LPARAM);
void  ShowMapError(int, char *);
int   scantable_lookup(char *);

struct PCScanCode * scantable_sclookup(int);
int GetKeymapLine ( char*, keytranslationentry_t *, int);

//-----------------------------------------------------
// Processes Keymap edit dialog messages.
//-----------------------------------------------------

BOOL CALLBACK KeyMapProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
//	PrintLogC("D: %04x %08x %08x",msg,wParam,lParam);
	switch (msg) {
	case WM_INITDIALOG:
        return InitKeymapDialog(hWnd);
	case WM_CLOSE:
		return EndDialog(hWnd,wParam);
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
        case IDC_KEYMAP_EXIT:
			return EndDialog(hWnd,wParam);
// TODO finish load,save,and set 
		case IDC_LOAD_KEYMAP:
		case IDC_SAVE_KEYMAP:
		case IDC_SET_CUST_KEYMAP:
	    	SetFocus(hEdit_PC);
            return TRUE;
		default:
			return Process_CoCoKey(LOWORD(wParam));
		}
	}
    return FALSE; 
}

//-----------------------------------------------------
// Read and process lines from the custom keymap file
//-----------------------------------------------------
int LoadCustomKeyMap(char* keymapfile)
{
    FILE *keymap;
    char buf[512];
    int  ndx  = 0;
    int  lnum = 0;

    // Open the keymap file. Abort operation if open fails.
    if ((keymap = fopen(keymapfile,"r")) == NULL) {
        if (! UserWarned) {
            sprintf(buf,"Keymap %s open failed\n",  keymapfile);
            MessageBox(0,buf,"Error",  0);
            UserWarned = 1;
        }
        return 1;
    }
    UserWarned = 0;

    // clear the custom keymap.
    memset ( keyTranslationsCustom, 0,
			 sizeof(keytranslationentry_t)*MAX_CTRANSTBLSIZ);

    // load keymap from file.
    while (fgets(buf,500,keymap)) {
        lnum++;
        if (GetKeymapLine(buf, &keyTranslationsCustom[ndx],lnum)==0) {
            ndx++;
            if (ndx > MAX_CTRANSTBLSIZ-1) { // save space for term
                ShowMapError(lnum,"Translation table full!");
                fclose(keymap);
                return 1;
            }
        }
    }

    fclose(keymap);
    return 0;
}

//-----------------------------------------------------
// Show message box for custom keymap load errors
//-----------------------------------------------------

void ShowMapError(int lnum, char *msg)
{
	if (UserWarned < 8) {
        char fullmsg[128];
        sprintf(fullmsg,"Line %d %s",  lnum,msg);
        MessageBox(0,fullmsg,"Error",  0);
	} else if (UserWarned == 8) {
        MessageBox(0,"Too many errors to show",  "Error",  0);
	}
    UserWarned++;
}

//-----------------------------------------------------
// Keyname lookup on PC keynames table
//-----------------------------------------------------
int scantable_lookup(char * keyname)
{
    int first = 0;
    int last = (sizeof(sctable)/sizeof(sctable[0]))-1;
    int mid = last / 2;
    int cmp = strncmp(keyname,"DIK_",  4);
    if (cmp != 0) return -1;
    while (first < last) {
        cmp = strcmp(keyname+4,sctable[mid].keyname);
        if (cmp == 0) return mid;
        if (cmp > 0) {
            first = mid + 1;
        } else {
            last = mid;
        }
        mid = (first+last)/2;
    }
    return -1;
}

//-----------------------------------------------------
// ScanCode lookup on PC keynames table
// Sequential scan is quick enough
//-----------------------------------------------------
struct PCScanCode * scantable_sclookup(int ScanCode)
{
	struct PCScanCode *p = sctable;
	while (p->ScanCode > 0) {
		if (p->ScanCode == ScanCode) return p;
		p++;
	}
    return NULL;
}

//-----------------------------------------------------
// Do lookup on CoCo keynames table
//-----------------------------------------------------
int cocotable_lookup(char * keyname)
{
    int first = 0;
    int last = (sizeof(cctable)/sizeof(cctable[0]))-1;
    int mid = last / 2;
    int cmp = strncmp(keyname,"COCO_",  5);
    if (cmp != 0) return -1;
    while (first < last) {
        cmp = strcmp(keyname+5,cctable[mid].keyname);
        if (cmp == 0) return mid;
        if (cmp > 0) {
            first = mid + 1;
        } else {
            last = mid;
        }
        mid = (first+last)/2;
    }
    return -1;
}

//-----------------------------------------------------
// Parse an line from keymap file and maybe load trans table entry
//-----------------------------------------------------
int GetKeymapLine ( char* line, keytranslationentry_t * trans, int lnum)
{
    char *p;
    int n;

    // pc scancode -> ScanCode1
    p = strtok(line, " \t\n\r");
    if ((p == NULL) || (*p == '#')) return 1; // comment

    n = scantable_lookup(p);
    if (n < 0) {
        ShowMapError(lnum,"PC Scancode not recognized");
        return 2;
    }
    trans->ScanCode1 = sctable[n].ScanCode;

    // pc modifier -> ScanCode2
    p = strtok(NULL, " \t\n\r");
    if (p == NULL) {
        ShowMapError(lnum,"PC Scancode modifier missing");
        return 2;
    }
    // Generate scancode for modifier 1=shift,2=ctrl,3=alt
    switch(*p) {
    case '0':
        trans->ScanCode2 = 0;
        break;
    case '1':
        trans->ScanCode2 = DIK_LSHIFT;
        break;
    case '2':
        trans->ScanCode2 = DIK_LCONTROL;
        break;
    case '3':
        trans->ScanCode2 = DIK_LALT;
        break;
    default:
        ShowMapError(lnum,"PC key modifier not recognized");
        return 2;
    }

    // CoCo Key -> Row1, Col1
    p = strtok(NULL, " \t\n\r");
    if (p == NULL) {
        ShowMapError(lnum,"CoCo keyname missing");
        return 2;
    }

    n = cocotable_lookup(p);
    if (n < 0) {
        ShowMapError(lnum,"CoCo keyname not recognized");
        return 2;
    }
    trans->Row1 = cctable[n].row;
    trans->Col1 = cctable[n].col;

    // CoCo Key modifer -> Row2, Col2
    p = strtok(NULL, " \t\n\r");

    // If modifier missing accept input anyway
    if (p == NULL) {
        trans->Row2 = 0;
        trans->Col2 = 0;
        return 0;
    }

    // Generate row and col for modifier 1=shift,2=ctrl,3=alt
	switch(*p) {
    case '0':
        trans->Row2 = 0;
        trans->Col2 = 0;
        break;
    case '1':
        trans->Row2 = 64;
        trans->Col2 = 7;
        break;
    case '2':
        trans->Row2 = 64;
        trans->Col2 = 4;
        break;
    case '3':
        trans->Row2 = 64;
        trans->Col2 = 3;
        break;
    default:
        ShowMapError(lnum,"CoCo key modifier not recognized");
        return 2;
    }
    return 0;
}

//-----------------------------------------------------
// Initialize key map dialog
//-----------------------------------------------------
BOOL InitKeymapDialog(HWND hWnd) 
{
    // Save dialog window handle as global
	hKeyMapDlg = hWnd;

	// Save handles for data display controls as globals
	hEdit_CC = GetDlgItem(hWnd,IDC_PCKEY_TXT);
	hEdit_PC = GetDlgItem(hWnd,IDC_CCKEY_TXT);
    hEdit_MapFile = GetDlgItem(hWnd,IDC_KEYMAP_FILE);

	// Initialize selected keys information
	CC_KeySelected = 0;
	CC_ModSelected = 0;
	PC_KeySelected = 0;
	PC_ModSelected = 0;
    CoCoKeySet = 0;
	CoCoModSet = 0;

    // keymap filename
	// TODO:  Make it real
	char str[64];
	sprintf(str, "Custom.keymap");
	SendMessage(hEdit_MapFile, WM_SETTEXT, 0, (LPARAM)str);

	// Subclass hEdit_PC to handle PC keyboard
	Edit_PCproc = (WNDPROC)GetWindowLongPtr(hEdit_PC, GWLP_WNDPROC);
	SetWindowLongPtr(hEdit_PC, GWLP_WNDPROC, (LONG_PTR)SubEdit_PCproc);
	SetFocus(hEdit_PC);

	return FALSE;
}

//-----------------------------------------------------
// Handle CoCo key button press
//-----------------------------------------------------
BOOL Process_CoCoKey(int BtnId)
{
	// Return FALSE if not a CoCo key button 
	if (BtnId < IDC_KEYBTN_FIRST) return FALSE;
	if (BtnId > IDC_KEYBTN_LAST) return FALSE;

	SetFocus(hEdit_PC);
	switch (BtnId) {
	case IDC_KEYBTN_RSHIFT:
	case IDC_KEYBTN_LSHIFT:
		CoCoModifier(1);
		break;
	case IDC_KEYBTN_CTRL:
		CoCoModifier(2);
        break;
	case IDC_KEYBTN_ALT:
		CoCoModifier(3);
        break;
	default:
    	if (CC_KeySelected != BtnId) {
			CC_KeySelected = BtnId;
		} else {
			CC_KeySelected = 0;
			CC_ModSelected = 0;
		}
	}
	ShowCoCoKey();
	return TRUE;
}

//-----------------------------------------------------
// Select or toggle coco shift, ctrl, or alt (1, 2, or 3)
//-----------------------------------------------------
void CoCoModifier(int Modifier)
{
	if (CC_ModSelected != Modifier) {
		CC_ModSelected = Modifier;
	} else {
		CC_ModSelected = 0;
		CC_KeySelected = 0;
	}
}

//-----------------------------------------------------
// Set up/down state of modifier key button(s)
//-----------------------------------------------------
void SetModKeyState (int mod, int state) 
{
    switch (mod) {
	case 1:
	    Button_SetState(GetDlgItem(hKeyMapDlg,IDC_KEYBTN_LSHIFT),state);
	    Button_SetState(GetDlgItem(hKeyMapDlg,IDC_KEYBTN_RSHIFT),state);
		break;
	case 2:
	    Button_SetState(GetDlgItem(hKeyMapDlg,IDC_KEYBTN_CTRL),state);
		break;
	case 3:
	    Button_SetState(GetDlgItem(hKeyMapDlg,IDC_KEYBTN_ALT),state);
    }
}

//-----------------------------------------------------
// Display CoCo keys 
//-----------------------------------------------------
void ShowCoCoKey()
{
	char str[64];
	char * keytxt = "";
	char * modtxt = "";
	struct CoCoKey *p;

	// set coco keyboard buttons 
	if (CC_KeySelected != CoCoKeySet) {
		if (CoCoKeySet) {
	       Button_SetState(GetDlgItem(hKeyMapDlg,CoCoKeySet),0);
        }
		if (CC_KeySelected) {
	       Button_SetState(GetDlgItem(hKeyMapDlg,CC_KeySelected),1);
		}
	    CoCoKeySet = CC_KeySelected;
	}
		  
	if (CC_ModSelected != CoCoModSet ) {
        SetModKeyState (CoCoModSet, 0);
		SetModKeyState (CC_ModSelected, 1);
		CoCoModSet = CC_ModSelected;
	}

	// Show coco keys names in editbox
	if (CC_KeySelected) {
//	    p = GetCoCoTable(); 
        p = cctable;
		while (p->id > 0) { 
			if (p->id == CC_KeySelected) {
		        if (p->label != NULL) {
			        keytxt = p->label;
		        } else {
			        keytxt = p->keyname;   
				}
				break;
			}
			p++; 
    	}
	}

	sprintf(str, " %s %s",ModName[CC_ModSelected],keytxt);
	SendMessage(hEdit_CC, WM_SETTEXT, 0, (LPARAM)str);
}

//-----------------------------------------------------
// Handle messages for PC key edit control. This should
// get all Keyboard key down events while dialog is running.
//-----------------------------------------------------

LRESULT CALLBACK 
SubEdit_PCproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
//	PrintLog("E: %04x %08x %08x",msg,wParam,lParam);
    int ScanCode;
    int Extended;
	switch (msg) {
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
        ScanCode=(lParam >> 16) & 0xFF;
        Extended=(lParam >> 24) & 1;
		if (Extended && (ScanCode>DIK_SCROLL)) ScanCode += 0x80;

	    switch (ScanCode) {
	    case DIK_LSHIFT:
	    case DIK_RSHIFT:
			SetPCmod(1);
			break;
		case DIK_LCONTROL:
	    case DIK_RCONTROL:
			SetPCmod(2);
			break;
	    case DIK_LMENU:
	    case DIK_RMENU:
			SetPCmod(3);
			break;
	    default:
		    PC_KeySelected = ScanCode;
			break;
	    }
		ShowPCkey();
		return 0;
	}
	// Stock control does everything else
	return (CallWindowProc(Edit_PCproc,hWnd,msg,wParam,lParam));
}

//-----------------------------------------------------
// Select or toggle PC shift, ctrl, or alt 
//-----------------------------------------------------
void SetPCmod(int ModNum)
{
	if (ModNum == PC_ModSelected) {
		PC_KeySelected = 0;
		PC_ModSelected = 0;
	} else {
		PC_ModSelected = ModNum;
	}
}

//-----------------------------------------------------
// Display PC keys names that are selected
//-----------------------------------------------------
void ShowPCkey()
{
	char str[64];
	char * keytxt = "";
	char * modtxt = "";
    struct PCScanCode * entry;
	if (PC_KeySelected>0) { 
		entry = scantable_sclookup(PC_KeySelected);
		if (entry == NULL) {
			keytxt = "";
			PC_KeySelected=0;
		} else {
			keytxt = entry->label;
			if (keytxt == NULL) keytxt = entry->keyname;
		}
	}
	sprintf(str, " %s %s",ModName[PC_ModSelected],keytxt);
	SendMessage(hEdit_PC, WM_SETTEXT, 0, (LPARAM)str);
}

//  TODO: Change CoCo key as per current map
//	keytranslationentry_t * keyTranslationsCustom;

