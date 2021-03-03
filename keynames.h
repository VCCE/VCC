/*****************************************************************************/
/*
Vcc Copyright 2015 by Joseph Forgione.
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
*/

/******************************************************************** 
 keynames.h

 This header file defines the keynames used by the custom keymap editor.

 It is included by keyboardEdit.c  

 ********************************************************************/

/******************************************************************** 
 * ScanCode table associates PC scan code text with scan code values
 * 
 * This establishes the PC keyboard scancodes that may be mapped to
 * CoCo keys.
 *
 * DIK_ is removed from keyname strings for brevity
 *
 * ScanCode names and values are defined in dinput.h
 *
 * This table is sorted alphabetically to facilitate binary lookups.
 *
 * Key labels are for display in keymap editor. If Key label is 
 * NULL the keyname is used as the label.
 ********************************************************************/

struct PCScanCode {
    char* keyname;           // Keyname in custom keymap file less "DIK_"
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
 * CoCoKey table used to convert CoCo key names into rollover table
 * row and columns.
 *
 * There is an entry for each physical key on the coco keyboard
 *
 * This table is sorted alphabetically by keyname to facilitate binary
 * searches.  If this table is changed the followind index must alse be
 * changed to match.
 *
 * COCO_ is removed from keyname strings for brevity
 * If Key label strings are NULL the keyname is used
 *
 ************************************************************************/
 
struct CoCoKey {
    char* keyname;      // Keyname in custom keymap file less "COCO_"
    char* label;        // Label for keymap editor display
    int id;             // Button resource ID on key edit dialog
    unsigned char row;  // row 0-6 (bit num for trans table)
    unsigned char col;  // col 0-7
};

static struct CoCoKey cctable[57] = // each coco key + null terminator
{
     { "0",         NULL,       IDC_KEYBTN_0,       4,0 },   //  0
     { "1",         NULL,       IDC_KEYBTN_1,       4,1 },   //  1
     { "2",         NULL,       IDC_KEYBTN_2,       4,2 },   //  2
     { "3",         NULL,       IDC_KEYBTN_3,       4,3 },   //  3
     { "4",         NULL,       IDC_KEYBTN_4,       4,4 },   //  4
     { "5",         NULL,       IDC_KEYBTN_5,       4,5 },   //  5
     { "6",         NULL,       IDC_KEYBTN_6,       4,6 },   //  6
     { "7",         NULL,       IDC_KEYBTN_7,       4,7 },   //  7
     { "8",         NULL,       IDC_KEYBTN_8,       5,0 },   //  8
     { "9",         NULL,       IDC_KEYBTN_9,       5,1 },   //  9
     { "A",         NULL,       IDC_KEYBTN_A,       0,1 },   //  A
     { "ALT",       NULL,       IDC_KEYBTN_ALT,     6,3 },   //  ALT 
     { "AT",        "@",        IDC_KEYBTN_AT,      0,0 },   //  @      
     { "B",         NULL,       IDC_KEYBTN_B,       0,2 },   //  B
     { "BREAK",     "Break",    IDC_KEYBTN_BRK,     6,2 },   //  BREAK
     { "C",         NULL,       IDC_KEYBTN_C,       0,3 },   //  C
     { "CLEAR",     "Clear",    IDC_KEYBTN_CLR,     6,1 },   //  CLEAR
     { "COLON",     "Colon",    IDC_KEYBTN_COLON,   5,2 },   //  :
     { "COMMA",     "Comma",    IDC_KEYBTN_COMMA,   5,4 },   //  ,
     { "CTRL",      NULL,       IDC_KEYBTN_CTRL,    6,4 },   //  CTRL
     { "D",         NULL,       IDC_KEYBTN_D,       0,4 },   //  D
     { "DOWN",      "Down",     IDC_KEYBTN_DOWN,    3,4 },   //  DOWN
     { "E",         NULL,       IDC_KEYBTN_E,       0,5 },   //  E
     { "ENTER",     "Enter",    IDC_KEYBTN_ENTER,   6,0 },   //  ENTER
     { "F",         NULL,       IDC_KEYBTN_F,       0,6 },   //  F
     { "F1",        NULL,       IDC_KEYBTN_F1,      6,5 },   //  F1
     { "F2",        NULL,       IDC_KEYBTN_F2,      6,6 },   //  F2
     { "G",         NULL,       IDC_KEYBTN_G,       0,7 },   //  G
     { "H",         NULL,       IDC_KEYBTN_H,       1,0 },   //  H
     { "I",         NULL,       IDC_KEYBTN_I,       1,1 },   //  I
     { "J",         NULL,       IDC_KEYBTN_J,       1,2 },   //  J
     { "K",         NULL,       IDC_KEYBTN_K,       1,3 },   //  K
     { "L",         NULL,       IDC_KEYBTN_L,       1,4 },   //  L
     { "LEFT",      "Left",     IDC_KEYBTN_LEFT,    3,5 },   //  LEFT  
     { "M",         NULL,       IDC_KEYBTN_M,       1,5 },   //  M
     { "MINUS",     "Minus",    IDC_KEYBTN_MINUS,   5,5 },   //  -
     { "N",         NULL,       IDC_KEYBTN_N,       1,6 },   //  N
     { "O",         NULL,       IDC_KEYBTN_O,       1,7 },   //  O
     { "P",         NULL,       IDC_KEYBTN_P,       2,0 },   //  P
     { "PERIOD",    "Period",   IDC_KEYBTN_PERIOD,  5,6 },   //  .
     { "Q",         NULL,       IDC_KEYBTN_Q,       2,1 },   //  Q
     { "R",         NULL,       IDC_KEYBTN_R,       2,2 },   //  R
     { "RIGHT",     "Right",    IDC_KEYBTN_RIGHT,   3,6 },   //  RIGHT
     { "S",         NULL,       IDC_KEYBTN_S,       2,3 },   //  S
     { "SEMICOLON", "Semicolon",IDC_KEYBTN_SEMI,    5,3 },   //  ;
     { "SHIFT",     "Shift",    IDC_KEYBTN_LSHIFT,  6,7 },   //  RSHIFT
     { "SLASH",     "Slash",    IDC_KEYBTN_SLASH,   5,7 },   //  /
     { "SPACE",     "Space",    IDC_KEYBTN_SPACE,   3,7 },   //  SPACE
     { "T",         NULL,       IDC_KEYBTN_T,       2,4 },   //  T
     { "U",         NULL,       IDC_KEYBTN_U,       2,5 },   //  U
     { "UP",        NULL,       IDC_KEYBTN_UP,      3,3 },   //  UP
     { "V",         NULL,       IDC_KEYBTN_V,       2,6 },   //  V
     { "W",         NULL,       IDC_KEYBTN_W,       2,7 },   //  W
     { "X",         NULL,       IDC_KEYBTN_X,       3,0 },   //  X     
     { "Y",         NULL,       IDC_KEYBTN_Y,       3,1 },   //  Y
     { "Z",         NULL,       IDC_KEYBTN_Z,       3,2 },   //  Z
     { NULL,        NULL,       NULL,               0,0 }    //  table end
};

// Row, col index for above cctable.  Must match table order above.
// This is used for fast lookup of CoCo keys by row and col.
static int cctable_ndx[56] = 
      {12, 10, 13, 15, 20, 22, 24, 27,  // row  1 @,A,B,C,D,E,F,G      
       28, 29, 30, 31, 32, 34, 36, 37,  // row  2 H,I,J,K,L,M,N,O
       38, 40, 41, 43, 48, 49, 51, 52,  // row  4 P,Q,R,S,T,U,V,W
       53, 54, 55, 50, 21, 33, 42, 47,  // row  8 X,Y,Z,UP,DOWN,LEFT,RIGHT,SPACE
        0,  1,  2,  3,  4,  5,  6,  7,  // row 16 0,1,2,3,4,5,6,7
        8,  9, 17, 44, 18, 35, 39, 46,  // row 32 8,9,COLON,SEMI,COMMA,MINUS,PERIOD 
       23, 16, 14, 11, 19, 25, 26, 45}; // row 64 ENTER,CLR,BRK,ALT,CTRL,F1,F1,SHIFT

