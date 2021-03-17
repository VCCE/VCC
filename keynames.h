/*************************************************************************
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
 

This header defines key names used by the custom keymap editor.
It is included by keyboardEdit.c and is private to it. 

*************************************************************************/
 
/*************************************************************************
    ScanCode table defines PC scancodes that can be mapped to CoCo keys.
 
    "DIK_" prefix is removed from keyname strings for brevity
 
    ScanCode values are defined in dinput.h
 
    This table is sorted alphabetically to facilitate binary lookups.
 
    label field is used for display in keymap editor. If string is NULL 
    the keyname field is used for this purpose.

*************************************************************************/

struct PCScanCode {
    char* keyname;           // Keyname in keymap file less "DIK_"
    char* label;             // Label for keymap editor display
    unsigned char ScanCode;  // Scan code from windows kb event
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
    {"DECIMAL",        "Decimal",    DIK_DECIMAL     },  // Numpad
    {"DELETE",         "Delete",     DIK_DELETE      },
    {"DIVIDE",         "Divide",     DIK_DIVIDE      },
    {"DOWN",           "Down",       DIK_DOWN        },
    {"E",              NULL,         DIK_E           },
    {"END",            "End",        DIK_END         },
    {"EQUALS",         "Equal",      DIK_EQUALS      },
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
    {"NUMPADMINUS",    "Subtract",   DIK_NUMPADMINUS }, 
    {"NUMPADPERIOD",   "Decimal",    DIK_NUMPADPERIOD}, 
    {"NUMPADPLUS",     "Add",        DIK_NUMPADPLUS  }, 
    {"NUMPADSLASH",    "Divide",     DIK_NUMPADSLASH }, 
    {"NUMPADSTAR",     "Multiply",   DIK_NUMPADSTAR  }, 
    {"O",              NULL,         DIK_O           }, 
    {"P",              NULL,         DIK_P           }, 
    {"PERIOD",         "Period",     DIK_PERIOD      }, 
    {"PGDN",           "PgDn",       DIK_PGDN        },
    {"PGUP",           "PgUp",       DIK_PGUP        },
    {"PRIOR",          "PgUp",       DIK_PRIOR       },
    {"Q",              NULL,         DIK_Q           },
    {"R",              NULL,         DIK_R           },
    {"RBRACKET",       "RBracket",   DIK_RBRACKET    },
    {"RCONTROL",       "Ctrl",       DIK_RCONTROL    },
    {"RETURN",         "Enter",      DIK_RETURN      },
    {"RIGHT",          "Right",      DIK_RIGHT       },
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
    {"V",              NULL,         DIK_V           },
    {"W",              NULL,         DIK_W           },
    {"X",              NULL,         DIK_X           },
    {"Y",              NULL,         DIK_Y           },
    {"Z",              NULL,         DIK_Z           },
    {NULL,             NULL,         0               }  // table end
};

// sctable_ndx maps unsigned byte scancode to entry in sctable 
// Table is built when index is first used
int sctable_ndx[256] = {0};

/*********************************************************************** 
  
   cctable used to convert CoCo key names into rollover table
   row and columns and to associate virtual keyboard buttons on
   the keymap editor to CoCo keys
  
   There is an entry for each physical key on the coco keyboard
  
   This table is sorted alphabetically by keyname to facilitate binary
   keyname lookup.  Hash on first char would have been quicker but this
   is easier to code and understand.
  
   "COCO_" previx is removed from keyname strings for brevity

   label field is used for display in keymap editor. If string is NULL 
   the keyname field is used for this purpose.
  
************************************************************************/
 
struct CoCoKey {
    char* keyname;      // Keyname in keymap file less "COCO_"
    char* label;        // Label for keymap editor display
    int id;             // Button ID for key edit dialog
    unsigned char row;  // row 0-6 (bit num for trans table)
    unsigned char col;  // col 0-7
};

static struct CoCoKey cctable[57] = // each coco key + null terminator
{
     { "0",         NULL,       IDC_KEYBTN_0,      4,0 }, //  0 0
     { "1",         NULL,       IDC_KEYBTN_1,      4,1 }, //  1 1
     { "2",         NULL,       IDC_KEYBTN_2,      4,2 }, //  2 2
     { "3",         NULL,       IDC_KEYBTN_3,      4,3 }, //  3 3
     { "4",         NULL,       IDC_KEYBTN_4,      4,4 }, //  4 4
     { "5",         NULL,       IDC_KEYBTN_5,      4,5 }, //  5 5
     { "6",         NULL,       IDC_KEYBTN_6,      4,6 }, //  6 6
     { "7",         NULL,       IDC_KEYBTN_7,      4,7 }, //  7 7
     { "8",         NULL,       IDC_KEYBTN_8,      5,0 }, //  8 8
     { "9",         NULL,       IDC_KEYBTN_9,      5,1 }, //  9 9
     { "A",         NULL,       IDC_KEYBTN_A,      0,1 }, // 10 A
     { "ALT",       NULL,       IDC_KEYBTN_ALT,    6,3 }, // 11 ALT 
     { "AT",        "@",        IDC_KEYBTN_AT,     0,0 }, // 12 @      
     { "B",         NULL,       IDC_KEYBTN_B,      0,2 }, // 13 B
     { "BREAK",     "Break",    IDC_KEYBTN_BRK,    6,2 }, // 14 BREAK
     { "C",         NULL,       IDC_KEYBTN_C,      0,3 }, // 15 C
     { "CLEAR",     "Clear",    IDC_KEYBTN_CLR,    6,1 }, // 16 CLEAR
     { "COLON",     "Colon",    IDC_KEYBTN_COLON,  5,2 }, // 17 :
     { "COMMA",     "Comma",    IDC_KEYBTN_COMMA,  5,4 }, // 18 ,
     { "CTRL",      NULL,       IDC_KEYBTN_CTRL,   6,4 }, // 19 CTRL
     { "D",         NULL,       IDC_KEYBTN_D,      0,4 }, // 20 D
     { "DOWN",      "Down",     IDC_KEYBTN_DOWN,   3,4 }, // 21 DOWN
     { "E",         NULL,       IDC_KEYBTN_E,      0,5 }, // 22 E
     { "ENTER",     "Enter",    IDC_KEYBTN_ENTER,  6,0 }, // 23 ENTER
     { "F",         NULL,       IDC_KEYBTN_F,      0,6 }, // 24 F
     { "F1",        NULL,       IDC_KEYBTN_F1,     6,5 }, // 25 F1
     { "F2",        NULL,       IDC_KEYBTN_F2,     6,6 }, // 26 F2
     { "G",         NULL,       IDC_KEYBTN_G,      0,7 }, // 27 G
     { "H",         NULL,       IDC_KEYBTN_H,      1,0 }, // 28 H
     { "I",         NULL,       IDC_KEYBTN_I,      1,1 }, // 29 I
     { "J",         NULL,       IDC_KEYBTN_J,      1,2 }, // 30 J
     { "K",         NULL,       IDC_KEYBTN_K,      1,3 }, // 31 K
     { "L",         NULL,       IDC_KEYBTN_L,      1,4 }, // 32 L
     { "LEFT",      "Left",     IDC_KEYBTN_LEFT,   3,5 }, // 33 LEFT  
     { "M",         NULL,       IDC_KEYBTN_M,      1,5 }, // 34 M
     { "MINUS",     "Minus",    IDC_KEYBTN_MINUS,  5,5 }, // 35 -
     { "N",         NULL,       IDC_KEYBTN_N,      1,6 }, // 36 N
     { "O",         NULL,       IDC_KEYBTN_O,      1,7 }, // 37 O
     { "P",         NULL,       IDC_KEYBTN_P,      2,0 }, // 38 P
     { "PERIOD",    "Period",   IDC_KEYBTN_PERIOD, 5,6 }, // 39 .
     { "Q",         NULL,       IDC_KEYBTN_Q,      2,1 }, // 40 Q
     { "R",         NULL,       IDC_KEYBTN_R,      2,2 }, // 41 R
     { "RIGHT",     "Right",    IDC_KEYBTN_RIGHT,  3,6 }, // 42 RIGHT
     { "S",         NULL,       IDC_KEYBTN_S,      2,3 }, // 43 S
     { "SEMICOLON", "Semicolon",IDC_KEYBTN_SEMI,   5,3 }, // 44 ;
     { "SHIFT",     "Shift",    IDC_KEYBTN_LSHIFT, 6,7 }, // 45 SHIFT
     { "SLASH",     "Slash",    IDC_KEYBTN_SLASH,  5,7 }, // 46 /
     { "SPACE",     "Space",    IDC_KEYBTN_SPACE,  3,7 }, // 47 SPACE
     { "T",         NULL,       IDC_KEYBTN_T,      2,4 }, // 48 T
     { "U",         NULL,       IDC_KEYBTN_U,      2,5 }, // 49 U
     { "UP",        NULL,       IDC_KEYBTN_UP,     3,3 }, // 50 UP
     { "V",         NULL,       IDC_KEYBTN_V,      2,6 }, // 51 V
     { "W",         NULL,       IDC_KEYBTN_W,      2,7 }, // 52 W
     { "X",         NULL,       IDC_KEYBTN_X,      3,0 }, // 53 X     
     { "Y",         NULL,       IDC_KEYBTN_Y,      3,1 }, // 54 Y
     { "Z",         NULL,       IDC_KEYBTN_Z,      3,2 }, // 55 Z
     { NULL,        NULL,       NULL,              0,0 }  // table end
};                                                             

// Index for above cctable. Allows lookup by 8 x row + col.
// Index maps rollover table index to cctable.
static int cctable_ndx[56] = {
    12, 10, 13, 15, 20, 22, 24, 27, // @, A, B, C, D, E, F, G
    28, 29, 30, 31, 32, 34, 36, 37, // H, I, J, K, L, M, N, O
    38, 40, 41, 43, 48, 49, 51, 52, // P, Q, R, S, T, U, V, W
    53, 54, 55, 50, 21, 33, 42, 47, // X, Y, Z, UP, DOWN, LEFT, RIGHT, SPACE
     0,  1,  2,  3,  4,  5,  6,  7, // 0, 1, 2, 3, 4, 5, 6, 7
     8,  9, 17, 44, 18, 35, 39, 46, // 8, 9, COLON, SEMI, COMMA, MINUS, PERIOD 
    23, 16, 14, 11, 19, 25, 26, 45  // ENT, CLR, BRK, ALT, CTRL, F1, F2, SHIFT
};

