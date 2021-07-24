/*****************************************************************************/
/*
	Copyright 2015 by Joseph Forgione
	This file is part of VCC (Virtual Color Computer).

	VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*

*/
/*****************************************************************************/
/*
	Keyboard layout data

	key translation tables used to convert keyboard oem scan codes / key 
	combinations into CoCo keyboard row/col values

*//*
    The row values are bitmasks (1,2,4,8,16,32,64)
    The column is a digit (0-7) as follows:
           0     1      2      3     4     5     6     7
          ---   ---   -----   ---   ---   ---   ---  -----
     1:    @     A      B      C     D     E     F     G
     2:    H     I      J      K     L     M     N     O
     4:    P     Q      R      S     T     U     V     W
     8:    X     Y      Z      UP   DWN   LFT   RGT  SPACE
    16:    0     1!     2"     3#    4$    5%    6&    7'
    32:   8(     9)     :*     ;+    ,<    -=    .>    /?
    64:   ENT   CLR  BRK/ESC  ALT    CTL   F1    F2  SHIFT
*//*

	ScanCode1 and ScanCode2 are used to determine what actual
	key presses are translated into a specific CoCo key

	Keyboard ScanCodes are from dinput.h

	The code expects SHIFTed entries in the table to use DIK_LSHIFT (not DIK_RSHIFT)
	The key handling code turns DIK_RSHIFT into DIK_LSHIFT

	These do not need to be in any particular order, 
	the code sorts them after they are copied to the run-time table
	each table is terminated at the first entry with ScanCode1+2 == 0

	PC Keyboard:
	+---------------------------------------------------------------------------------+
	| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
	|                                                                                 |
	| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
	| [  Tab][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
	| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
	| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
	| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
	+---------------------------------------------------------------------------------+

*/
/*****************************************************************************/

#include "keyboardLayout.h"

#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>


/*****************************************************************************/
/**
	Original VCC key translation table for DECB

	VCC BASIC Keyboard:

	+--------------------------------------------------------------------------------+
	[  ][F1][F2][  ][  ][Rst][RGB][  ][Thr][Pwr][StB][FSc][  ]   [    ][   ][    ]   |
    |                                                                                |
	| [ ][1!][2"][3#][4$][5%][6&][7'][8(][9)][0 ][:*][-=][BkSpc]   [    ][Clr][    ] |
	| [    ][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [    ][Esc][    ] |
	| [ Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:][  ][Enter]                     |
	| [ Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
	| [Cntl][   ][Alt][       Space       ][ @ ][   ][   ][Cntl]   [LftA][DnA][RgtA] |
	+--------------------------------------------------------------------------------+
	*/
keytranslationentry_t keyTranslationsCoCo[] =
{
	// ScanCode1,     ScanCode2,      Row1,  Col1,  Row2, Col2    Char  
	{ DIK_A,          0,              1,     1,     0,    0 }, //   A
	{ DIK_B,          0,              1,     2,     0,    0 }, //   B
	{ DIK_C,          0,              1,     3,     0,    0 }, //   C
	{ DIK_D,          0,              1,     4,     0,    0 }, //   D
	{ DIK_E,          0,              1,     5,     0,    0 }, //   E
	{ DIK_F,          0,              1,     6,     0,    0 }, //   F
	{ DIK_G,          0,              1,     7,     0,    0 }, //   G
	{ DIK_H,          0,              2,     0,     0,    0 }, //   H
	{ DIK_I,          0,              2,     1,     0,    0 }, //   I
	{ DIK_J,          0,              2,     2,     0,    0 }, //   J
	{ DIK_K,          0,              2,     3,     0,    0 }, //   K
	{ DIK_L,          0,              2,     4,     0,    0 }, //   L
	{ DIK_M,          0,              2,     5,     0,    0 }, //   M
	{ DIK_N,          0,              2,     6,     0,    0 }, //   N
	{ DIK_O,          0,              2,     7,     0,    0 }, //   O
	{ DIK_P,          0,              4,     0,     0,    0 }, //   P
	{ DIK_Q,          0,              4,     1,     0,    0 }, //   Q
	{ DIK_R,          0,              4,     2,     0,    0 }, //   R
	{ DIK_S,          0,              4,     3,     0,    0 }, //   S
	{ DIK_T,          0,              4,     4,     0,    0 }, //   T
	{ DIK_U,          0,              4,     5,     0,    0 }, //   U
	{ DIK_V,          0,              4,     6,     0,    0 }, //   V
	{ DIK_W,          0,              4,     7,     0,    0 }, //   W
	{ DIK_X,          0,              8,     0,     0,    0 }, //   X
	{ DIK_Y,          0,              8,     1,     0,    0 }, //   Y
	{ DIK_Z,          0,              8,     2,     0,    0 }, //   Z
	{ DIK_0,          0,             16,     0,     0,    0 }, //   0
	{ DIK_1,          0,             16,     1,     0,    0 }, //   1
	{ DIK_2,          0,             16,     2,     0,    0 }, //   2
	{ DIK_3,          0,             16,     3,     0,    0 }, //   3
	{ DIK_4,          0,             16,     4,     0,    0 }, //   4
	{ DIK_5,          0,             16,     5,     0,    0 }, //   5
	{ DIK_6,          0,             16,     6,     0,    0 }, //   6
	{ DIK_7,          0,             16,     7,     0,    0 }, //   7
	{ DIK_8,          0,             32,     0,     0,    0 }, //   8
	{ DIK_9,          0,             32,     1,     0,    0 }, //   9
	{ DIK_1,          DIK_LSHIFT,    16,     1,    64,    7 }, //   !
	{ DIK_2,          DIK_LSHIFT,    16,     2,    64,    7 }, //   "
	{ DIK_3,          DIK_LSHIFT,    16,     3,    64,    7 }, //   #
	{ DIK_4,          DIK_LSHIFT,    16,     4,    64,    7 }, //   $
	{ DIK_5,          DIK_LSHIFT,    16,     5,	   64,    7 }, //   %
	{ DIK_6,          DIK_LSHIFT,    16,     6,    64,    7 }, //   &
	{ DIK_7,          DIK_LSHIFT,    16,     7,    64,    7 }, //   '
	{ DIK_8,          DIK_LSHIFT,    32,     0,    64,    7 }, //   (
	{ DIK_9,          DIK_LSHIFT,    32,     1,    64,    7 }, //   )
	{ DIK_0,          DIK_LSHIFT,    16,     0,    64,    7 }, //   CAPS LOCK (DECB SHIFT-0, OS-9 CTRL-0 does not need ot be emulated specifically)
	{ DIK_SPACE,      0,              8,     7,     0,    0 }, //   SPACE

	{ DIK_COMMA,      0,             32,     4,     0,    0 }, //   ,
	{ DIK_PERIOD,     0,             32,     6,     0,    0 }, //   .
	{ DIK_SLASH,      DIK_LSHIFT,    32,     7,    64,    7 }, //   ?
	{ DIK_SLASH,      0,             32,     7,     0,    0 }, //   /
	{ DIK_MINUS,      DIK_LSHIFT,    32,     2,    64,    7 }, //   *
	{ DIK_MINUS,      0,             32,     2,     0,    0 }, //   :
	{ DIK_SEMICOLON,  DIK_LSHIFT,    32,     3,    64,    7 }, //   +
	{ DIK_SEMICOLON,  0,             32,     3,     0,    0 }, //   ;
	{ DIK_EQUALS,     DIK_LSHIFT,    32,     5,    64,    7 }, //   =
	{ DIK_EQUALS,     0,             32,     5,     0,    0 }, //   -
//	{ DIK_GRAVE,      DIK_LSHIFT,    16,     3,    64,    4 }, //   ~ (tilde) (CoCo CTRL-3)

	// added
	{ DIK_UPARROW,    0,              8,     3,     0,    0 }, //   UP
	{ DIK_DOWNARROW,  0,              8,     4,     0,    0 }, //   DOWN
	{ DIK_LEFTARROW,  0,              8,     5,     0,    0 }, //   LEFT
	{ DIK_RIGHTARROW, 0,              8,     6,     0,    0 }, //   RIGHT

	{ DIK_NUMPAD8,    0,              8,     3,     0,    0 }, //   UP
	{ DIK_NUMPAD2,    0,              8,     4,     0,    0 }, //   DOWN
	{ DIK_NUMPAD4,    0,              8,     5,     0,    0 }, //   LEFT
	{ DIK_NUMPAD6,    0,              8,     6,     0,    0 }, //   RIGHT

	{ DIK_RETURN,     0,             64,     0,     0,    0 }, //   ENTER
	{ DIK_NUMPAD7,    0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ DIK_HOME,       0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ DIK_NUMPAD1,    0,             64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ DIK_END,        0,             64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ DIK_F1,         0,             64,     5,     0,    0 }, //   F1
	{ DIK_F2,         0,             64,     6,     0,    0 }, //   F2
	{ DIK_BACK,       0,              8,     5,     0,    0 }, //   BACKSPACE -> CoCo left arrow

//	{ DIK_CAPSLOCK,   0,             64,     4,    16,    0 }, //   CAPS LOCK (CoCo CTRL-0 for OS-9)
	{ DIK_CAPSLOCK,   0,             64,     7,    16,    0 }, //   CAPS LOCK (CoCo SHIFT-0 for DECB)

	// these produce the square bracket characters in DECB 
	// but it does not match what the real CoCo does
	//{ DIK_LBRACKET,   0,           64,     7,    8,    4 }, //   [
	//{ DIK_RBRACKET,   0,           64,     7,    8,    6 }, //   ]
	// added from OS-9 layout
	{ DIK_LBRACKET,   0,             64,     4,    32,    0 }, //   [ (CoCo CTRL 8)
	{ DIK_RBRACKET,   0,             64,     4,    32,    1 }, //   ] (CoCo CTRL 9)
	{ DIK_LBRACKET,   DIK_LSHIFT,    64,     4,    32,    4 }, //   { (CoCo CTRL ,)
	{ DIK_RBRACKET,   DIK_LSHIFT,    64,     4,    32,    6 }, //   } (CoCo CTRL .)

//	{ DIK_SCROLL,     0,              1,     0,    64,    7 }, //   SCROLL (CoCo SHIFT @)
	{ DIK_RALT,       0,              1,     0,     0,    0 }, //   @

	{ DIK_LALT,       0,             64,     3,     0,    0 }, //   ALT
	{ DIK_LCONTROL,   0,             64,     4,     0,    0 }, //   CTRL
	{ DIK_LSHIFT,     0,             64,     7,     0,    0 }, //   SHIFT (the code converts DIK_RSHIFT to DIK_LSHIFT)

	{ 0,              0,              0,     0,     0,    0 }  // terminator
};

/*****************************************************************************/
/**
	Original VCC key translation table for OS-9

	PC Keyboard:
	+---------------------------------------------------------------------------------+
	| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
	|                                                                                 |
	| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
	| [  Tab][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
	| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
	| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
	| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
	+---------------------------------------------------------------------------------+

	VCC OS-9 Keyboard

	+---------------------------------------------------------------------------------+
	| [BRK][F1][F2][  ][  ][Rst][RGB][  ][Thr][Pwr][StB][FSc][BRK] [    ][   ][    ]  |
	|                                                                                 |
	| [`][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [INST][Clr][PgUp]  |		INST=<CNTRL><R-ARROW>	Home=<CLEAR>		PgUp=<SHFT><U-ARROW>
	| [    ][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [DEL ][EOL][PgDn]  |		DEL=<CNTRL><L-ARROW>	END=<SHFT><R-ARROW>	PgDn=<SHFT><D-ARROW>
	| [ Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                      |
	| [ Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]        |
	| [Cntl][   ][Alt][       Space       ][Alt][   ][   ][Cntl]   [LftA][DnA][RgtA]  |
	+---------------------------------------------------------------------------------+
	*/
keytranslationentry_t keyTranslationsNatural[] =
{
	// ScanCode1,     ScanCode2,      Row1,  Col1,  Row2, Col2    Char  
	{ DIK_A,          0,              1,     1,     0,    0 }, //   A
	{ DIK_B,          0,              1,     2,     0,    0 }, //   B
	{ DIK_C,          0,              1,     3,     0,    0 }, //   C
	{ DIK_D,          0,              1,     4,     0,    0 }, //   D
	{ DIK_E,          0,              1,     5,     0,    0 }, //   E
	{ DIK_F,          0,              1,     6,     0,    0 }, //   F
	{ DIK_G,          0,              1,     7,     0,    0 }, //   G
	{ DIK_H,          0,              2,     0,     0,    0 }, //   H
	{ DIK_I,          0,              2,     1,     0,    0 }, //   I
	{ DIK_J,          0,              2,     2,     0,    0 }, //   J
	{ DIK_K,          0,              2,     3,     0,    0 }, //   K
	{ DIK_L,          0,              2,     4,     0,    0 }, //   L
	{ DIK_M,          0,              2,     5,     0,    0 }, //   M
	{ DIK_N,          0,              2,     6,     0,    0 }, //   N
	{ DIK_O,          0,              2,     7,     0,    0 }, //   O
	{ DIK_P,          0,              4,     0,     0,    0 }, //   P
	{ DIK_Q,          0,              4,     1,     0,    0 }, //   Q
	{ DIK_R,          0,              4,     2,     0,    0 }, //   R
	{ DIK_S,          0,              4,     3,     0,    0 }, //   S
	{ DIK_T,          0,              4,     4,     0,    0 }, //   T
	{ DIK_U,          0,              4,     5,     0,    0 }, //   U
	{ DIK_V,          0,              4,     6,     0,    0 }, //   V
	{ DIK_W,          0,              4,     7,     0,    0 }, //   W
	{ DIK_X,          0,              8,     0,     0,    0 }, //   X
	{ DIK_Y,          0,              8,     1,     0,    0 }, //   Y
	{ DIK_Z,          0,              8,     2,     0,    0 }, //   Z
	{ DIK_0,          0,             16,     0,     0,    0 }, //   0
	{ DIK_1,          0,             16,     1,     0,    0 }, //   1
	{ DIK_2,          0,             16,     2,     0,    0 }, //   2
	{ DIK_3,          0,             16,     3,     0,    0 }, //   3
	{ DIK_4,          0,             16,     4,     0,    0 }, //   4
	{ DIK_5,          0,             16,     5,     0,    0 }, //   5
	{ DIK_6,          0,             16,     6,     0,    0 }, //   6
	{ DIK_7,          0,             16,     7,     0,    0 }, //   7
	{ DIK_8,          0,             32,     0,     0,    0 }, //   8
	{ DIK_9,          0,             32,     1,     0,    0 }, //   9
	{ DIK_1,          DIK_LSHIFT,    16,     1,    64,    7 }, //   !
	{ DIK_2,          DIK_LSHIFT,     1,     0,     0,    0 }, //   @
	{ DIK_3,          DIK_LSHIFT,    16,     3,    64,    7 }, //   #
	{ DIK_4,          DIK_LSHIFT,    16,     4,    64,    7 }, //   $
	{ DIK_5,          DIK_LSHIFT,    16,     5,	   64,    7 }, //   %
	{ DIK_6,          DIK_LSHIFT,    16,     7,    64,    4 }, //   ^ (CoCo CTRL 7)
	{ DIK_7,          DIK_LSHIFT,    16,     6,    64,    7 }, //   &
	{ DIK_8,          DIK_LSHIFT,    32,     2,    64,    7 }, //   *
	{ DIK_9,          DIK_LSHIFT,    32,     0,    64,    7 }, //   (
	{ DIK_0,          DIK_LSHIFT,    32,     1,    64,    7 }, //   )

	{ DIK_SEMICOLON,  0,             32,     3,     0,    0 }, //   ;
	{ DIK_SEMICOLON,  DIK_LSHIFT,    32,     2,     0,    0 }, //   :

	{ DIK_APOSTROPHE, 0,             16,     7,    64,    7 }, //   '
	{ DIK_APOSTROPHE, DIK_LSHIFT,    16,     2,    64,    7 }, //   "

	{ DIK_COMMA,      0,             32,     4,     0,    0 }, //   ,
	{ DIK_PERIOD,     0,             32,     6,     0,    0 }, //   .
	{ DIK_SLASH,      DIK_LSHIFT,    32,     7,    64,    7 }, //   ?
	{ DIK_SLASH,      0,             32,     7,     0,    0 }, //   /
	{ DIK_EQUALS,     DIK_LSHIFT,    32,     3,    64,    7 }, //   +
	{ DIK_EQUALS,     0,             32,     5,    64,    7 }, //   =
	{ DIK_MINUS,      0,             32,     5,     0,    0 }, //   -
	{ DIK_MINUS,      DIK_LSHIFT,    32,     5,    64,    4 }, //   _ (underscore) (CoCo CTRL -)

	// added
	{ DIK_UPARROW,    0,              8,     3,     0,    0 }, //   UP
	{ DIK_DOWNARROW,  0,              8,     4,     0,    0 }, //   DOWN
	{ DIK_LEFTARROW,  0,              8,     5,     0,    0 }, //   LEFT
	{ DIK_RIGHTARROW, 0,              8,     6,     0,    0 }, //   RIGHT

	{ DIK_NUMPAD8,    0,              8,     3,     0,    0 }, //   UP
	{ DIK_NUMPAD2,    0,              8,     4,     0,    0 }, //   DOWN
	{ DIK_NUMPAD4,    0,              8,     5,     0,    0 }, //   LEFT
	{ DIK_NUMPAD6,    0,              8,     6,     0,    0 }, //   RIGHT
	{ DIK_SPACE,      0,              8,     7,     0,    0 }, //   SPACE

	{ DIK_RETURN,     0,             64,     0,     0,    0 }, //   ENTER
	{ DIK_NUMPAD7,    0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ DIK_HOME,       0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ DIK_ESCAPE,     0,             64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ DIK_F12,        0,             64,     2,     0,    0 }, //   Alternate ESCAPE (BREAK) (fixes <CNTRL><BRK> sequence)
	{ DIK_NUMPAD1,    0,             64,     7,     8,    6 }, //   END OF LINE (SHIFT)(RIGHT)
	{ DIK_END,        0,             64,     7,     8,    6 }, //   END OF LINE (SHIFT)(RIGHT)
	{ DIK_NUMPADPERIOD, 0,           64,     4,     8,    5 }, //   DELETE (CTRL)(LEFT)
	{ DIK_NUMPAD0,    0,             64,     4,     8,    6 }, //   INSERT (CTRL)(RIGHT)
	{ DIK_NUMPAD9,    0,             64,     7,     8,    3 }, //   PAGEUP (SHFT)(UP)
	{ DIK_NUMPAD3,    0,             64,     7,     8,    4 }, //   PAGEDOWN (SHFT)(DOWN)
	{ DIK_F1,         0,             64,     5,     0,    0 }, //   F1
	{ DIK_F2,         0,             64,     6,     0,    0 }, //   F2
	{ DIK_BACK,       0,              8,     5,     0,    0 }, //   BACKSPACE -> CoCo left arrow

	{ DIK_LBRACKET,   0,             64,     4,    32,    0 }, //   [ (CoCo CTRL 8)
	{ DIK_RBRACKET,   0,             64,     4,    32,    1 }, //   ] (CoCo CTRL 9)
	{ DIK_LBRACKET,   DIK_LSHIFT,    64,     4,    32,    4 }, //   { (CoCo CTRL ,)
	{ DIK_RBRACKET,   DIK_LSHIFT,    64,     4,    32,    6 }, //   } (CoCo CTRL .)

	{ DIK_BACKSLASH,  0,             32,     7,    64,    4 }, //   '\' (Back slash) (CoCo CTRL /)
	{ DIK_BACKSLASH,  DIK_LSHIFT,    16,     1,    64,    4 }, //   | (Pipe) (CoCo CTRL 1)
	{ DIK_GRAVE,      DIK_LSHIFT,    16,     3,    64,    4 }, //   ~ (tilde) (CoCo CTRL 3)

	{ DIK_CAPSLOCK,   0,             64,     4,    16,    0 }, //   CAPS LOCK (CoCo CTRL 0 for OS-9)
//	{ DIK_CAPSLOCK,   0,             64,     7,    16,    0 }, //   CAPS LOCK (CoCo Shift 0 for DECB)

//	{ DIK_SCROLL,     0,              1,     0,    64,    7 }, //   SCROLL (CoCo SHIFT @)

	{ DIK_LALT,       0,             64,     3,     0,    0 }, //   ALT
	{ DIK_LCONTROL,   0,             64,     4,     0,    0 }, //   CTRL
	{ DIK_LSHIFT,     0,             64,     7,     0,    0 }, //   SHIFT (the code converts DIK_RSHIFT to DIK_LSHIFT)

	{ 0,              0,              0,     0,     0,    0 }  // terminator
};

/*****************************************************************************/
/**
Compact natural key translation table (no number pad)

PC Keyboard:
+---------------------------------------------------------------------------------+
| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
|                                                                                 |
| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
| [     ][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
+---------------------------------------------------------------------------------+

Compact natural Keyboard

+-----------------------------------------------------------------------------------+
| [  ][F1][F2][  ][  ][Rst][RGB][  ][Thr][Pwr][StB][FSc][  ]   [    ][   ][    ]    |
|                                                                                   |
| [BRK~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [    ][   ][    ] |
|    [ CLR][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [    ][   ][    ] |
|    [ Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
|    [ Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
|    [Cntl][   ][Alt][       Space       ][   ][   ][   ][Cntl]   [LftA][DnA][RgtA] |
+-----------------------------------------------------------------------------------+

Differences from Natural layout:

	CLR				TAB
	ESC/BREAK		GRAVE (`)
*/
keytranslationentry_t keyTranslationsCompact[] =
{
	// ScanCode1,     ScanCode2,      Row1,  Col1,  Row2, Col2    Char  
	{ DIK_A,          0,              1,     1,     0,    0 }, //   A
	{ DIK_B,          0,              1,     2,     0,    0 }, //   B
	{ DIK_C,          0,              1,     3,     0,    0 }, //   C
	{ DIK_D,          0,              1,     4,     0,    0 }, //   D
	{ DIK_E,          0,              1,     5,     0,    0 }, //   E
	{ DIK_F,          0,              1,     6,     0,    0 }, //   F
	{ DIK_G,          0,              1,     7,     0,    0 }, //   G
	{ DIK_H,          0,              2,     0,     0,    0 }, //   H
	{ DIK_I,          0,              2,     1,     0,    0 }, //   I
	{ DIK_J,          0,              2,     2,     0,    0 }, //   J
	{ DIK_K,          0,              2,     3,     0,    0 }, //   K
	{ DIK_L,          0,              2,     4,     0,    0 }, //   L
	{ DIK_M,          0,              2,     5,     0,    0 }, //   M
	{ DIK_N,          0,              2,     6,     0,    0 }, //   N
	{ DIK_O,          0,              2,     7,     0,    0 }, //   O
	{ DIK_P,          0,              4,     0,     0,    0 }, //   P
	{ DIK_Q,          0,              4,     1,     0,    0 }, //   Q
	{ DIK_R,          0,              4,     2,     0,    0 }, //   R
	{ DIK_S,          0,              4,     3,     0,    0 }, //   S
	{ DIK_T,          0,              4,     4,     0,    0 }, //   T
	{ DIK_U,          0,              4,     5,     0,    0 }, //   U
	{ DIK_V,          0,              4,     6,     0,    0 }, //   V
	{ DIK_W,          0,              4,     7,     0,    0 }, //   W
	{ DIK_X,          0,              8,     0,     0,    0 }, //   X
	{ DIK_Y,          0,              8,     1,     0,    0 }, //   Y
	{ DIK_Z,          0,              8,     2,     0,    0 }, //   Z
	{ DIK_0,          0,             16,     0,     0,    0 }, //   0
	{ DIK_1,          0,             16,     1,     0,    0 }, //   1
	{ DIK_2,          0,             16,     2,     0,    0 }, //   2
	{ DIK_3,          0,             16,     3,     0,    0 }, //   3
	{ DIK_4,          0,             16,     4,     0,    0 }, //   4
	{ DIK_5,          0,             16,     5,     0,    0 }, //   5
	{ DIK_6,          0,             16,     6,     0,    0 }, //   6
	{ DIK_7,          0,             16,     7,     0,    0 }, //   7
	{ DIK_8,          0,             32,     0,     0,    0 }, //   8
	{ DIK_9,          0,             32,     1,     0,    0 }, //   9
	{ DIK_1,          DIK_LSHIFT,    16,     1,    64,    7 }, //   !
	{ DIK_2,          DIK_LSHIFT,     1,     0,     0,    0 }, //   @
	{ DIK_3,          DIK_LSHIFT,    16,     3,    64,    7 }, //   #
	{ DIK_4,          DIK_LSHIFT,    16,     4,    64,    7 }, //   $
	{ DIK_5,          DIK_LSHIFT,    16,     5,	   64,    7 }, //   %
	{ DIK_6,          DIK_LSHIFT,    16,     7,    64,    4 }, //   ^ (CoCo CTRL 7)
	{ DIK_7,          DIK_LSHIFT,    16,     6,    64,    7 }, //   &
	{ DIK_8,          DIK_LSHIFT,    32,     2,    64,    7 }, //   *
	{ DIK_9,          DIK_LSHIFT,    32,     0,    64,    7 }, //   (
	{ DIK_0,          DIK_LSHIFT,    32,     1,    64,    7 }, //   )

	{ DIK_SEMICOLON,  0,             32,     3,     0,    0 }, //   ;
	{ DIK_SEMICOLON,  DIK_LSHIFT,    32,     2,     0,    0 }, //   :

	{ DIK_APOSTROPHE, 0,             16,     7,    64,    7 }, //   '
	{ DIK_APOSTROPHE, DIK_LSHIFT,    16,     2,    64,    7 }, //   "

	{ DIK_COMMA,      0,             32,     4,     0,    0 }, //   ,
	{ DIK_PERIOD,     0,             32,     6,     0,    0 }, //   .
	{ DIK_SLASH,      DIK_LSHIFT,    32,     7,    64,    7 }, //   ?
	{ DIK_SLASH,      0,             32,     7,     0,    0 }, //   /
	{ DIK_EQUALS,     DIK_LSHIFT,    32,     3,    64,    7 }, //   +
	{ DIK_EQUALS,     0,             32,     5,    64,    7 }, //   =
	{ DIK_MINUS,      0,             32,     5,     0,    0 }, //   -
	{ DIK_MINUS,      DIK_LSHIFT,    32,     5,    64,    4 }, //   _ (underscore) (CoCo CTRL -)

															   // added
	{ DIK_UPARROW,    0,              8,     3,     0,    0 }, //   UP
	{ DIK_DOWNARROW,  0,              8,     4,     0,    0 }, //   DOWN
	{ DIK_LEFTARROW,  0,              8,     5,     0,    0 }, //   LEFT
	{ DIK_RIGHTARROW, 0,              8,     6,     0,    0 }, //   RIGHT

	{ DIK_NUMPAD8,    0,              8,     3,     0,    0 }, //   UP
	{ DIK_NUMPAD2,    0,              8,     4,     0,    0 }, //   DOWN
	{ DIK_NUMPAD4,    0,              8,     5,     0,    0 }, //   LEFT
	{ DIK_NUMPAD6,    0,              8,     6,     0,    0 }, //   RIGHT
	{ DIK_SPACE,      0,              8,     7,     0,    0 }, //   SPACE

	{ DIK_RETURN,     0,             64,     0,     0,    0 }, //   ENTER
	{ DIK_TAB,        0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ DIK_GRAVE,      0,             64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ DIK_F1,         0,             64,     5,     0,    0 }, //   F1
	{ DIK_F2,         0,             64,     6,     0,    0 }, //   F2
	{ DIK_BACK,       0,              8,     5,     0,    0 }, //   BACKSPACE -> CoCo left arrow

	{ DIK_LBRACKET,   0,             64,     4,    32,    0 }, //   [ (CoCo CTRL 8)
	{ DIK_RBRACKET,   0,             64,     4,    32,    1 }, //   ] (CoCo CTRL 9)
	{ DIK_LBRACKET,   DIK_LSHIFT,    64,     4,    32,    4 }, //   { (CoCo CTRL ,)
	{ DIK_RBRACKET,   DIK_LSHIFT,    64,     4,    32,    6 }, //   } (CoCo CTRL .)

	{ DIK_BACKSLASH,  0,             32,     7,    64,    4 }, //   '\' (Back slash) (CoCo CTRL /)
	{ DIK_BACKSLASH,  DIK_LSHIFT,    16,     1,    64,    4 }, //   | (Pipe) (CoCo CTRL 1)
	{ DIK_GRAVE,      DIK_LSHIFT,    16,     3,    64,    4 }, //   ~ (tilde) (CoCo CTRL 3)

	{ DIK_CAPSLOCK,   0,             64,     4,    16,    0 }, //   CAPS LOCK (CoCo CTRL 0 for OS-9)
//	{ DIK_CAPSLOCK,   0,             64,     7,    16,    0 }, //   CAPS LOCK (CoCo Shift 0 for DECB)

//	{ DIK_SCROLL,     0,              1,     0,    64,    7 }, //   SCROLL (CoCo SHIFT @)

	{ DIK_LALT,       0,             64,     3,     0,    0 }, //   ALT
	{ DIK_LCONTROL,   0,             64,     4,     0,    0 }, //   CTRL
	{ DIK_LSHIFT,     0,             64,     7,     0,    0 }, //   SHIFT (the code converts DIK_RSHIFT to DIK_LSHIFT)

	{ 0,              0,              0,     0,     0,    0 }  // terminator
};

/*****************************************************************************/
/*****************************************************************************/
/**
Original VCC key translation table for Custom layout

PC Keyboard:
+---------------------------------------------------------------------------------+
| [Esc]   [F1][F2][F3][F4][F5][F6][F7][F8][F9][F10][F11][F12]   [Prnt][Scr][Paus] |
|                                                                                 |
| [`~][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [Inst][Hom][PgUp] |
| [  Tab][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [Dlet][End][PgDn] |
| [  Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                     |
| [  Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]       |
| [Cntl][Win][Alt][        Space       ][Alt][Win][Prp][Cntl]   [LftA][DnA][RgtA] |
+---------------------------------------------------------------------------------+

VCC Custom Keyboard

+---------------------------------------------------------------------------------+
| [Esc][F1][F2][  ][  ][Rst][RGB][  ][Thr][Pwr][StB][FSc][  ]  [    ][   ][    ]  |
|                                                                                 |
| [`][1!][2@][3#][4$][5%][6^][7&][8*][9(][0]][-_][=+][BkSpc]   [INST][Clr][PgUp]  |
| [    ][Qq][Ww][Ee][Rr][Tt][Yy][Uu][Ii][Oo][Pp][[{][]}][\|]   [DEL ][EOL][PgDn]  |
| [ Caps][Aa][Ss][Dd][Ff][Gg][Hh][Jj][Kk][Ll][;:]['"][Enter]                      |
| [ Shift ][Zz][Xx][Cc][Vv][Bb][Nn][Mm][,<][.>][/?][ Shift ]         [UpA]        |
| [Cntl][   ][Alt][       Space       ][Alt][   ][   ][Cntl]   [LftA][DnA][RgtA]  |
+---------------------------------------------------------------------------------+
*/
keytranslationentry_t keyTranslationsCustom[MAX_CTRANSTBLSIZ] =
{
	// ScanCode1,     ScanCode2,      Row1,  Col1,  Row2, Col2    Char  
	{ DIK_A,          0,              1,     1,     0,    0 }, //   A
	{ DIK_B,          0,              1,     2,     0,    0 }, //   B
	{ DIK_C,          0,              1,     3,     0,    0 }, //   C
	{ DIK_D,          0,              1,     4,     0,    0 }, //   D
	{ DIK_E,          0,              1,     5,     0,    0 }, //   E
	{ DIK_F,          0,              1,     6,     0,    0 }, //   F
	{ DIK_G,          0,              1,     7,     0,    0 }, //   G
	{ DIK_H,          0,              2,     0,     0,    0 }, //   H
	{ DIK_I,          0,              2,     1,     0,    0 }, //   I
	{ DIK_J,          0,              2,     2,     0,    0 }, //   J
	{ DIK_K,          0,              2,     3,     0,    0 }, //   K
	{ DIK_L,          0,              2,     4,     0,    0 }, //   L
	{ DIK_M,          0,              2,     5,     0,    0 }, //   M
	{ DIK_N,          0,              2,     6,     0,    0 }, //   N
	{ DIK_O,          0,              2,     7,     0,    0 }, //   O
	{ DIK_P,          0,              4,     0,     0,    0 }, //   P
	{ DIK_Q,          0,              4,     1,     0,    0 }, //   Q
	{ DIK_R,          0,              4,     2,     0,    0 }, //   R
	{ DIK_S,          0,              4,     3,     0,    0 }, //   S
	{ DIK_T,          0,              4,     4,     0,    0 }, //   T
	{ DIK_U,          0,              4,     5,     0,    0 }, //   U
	{ DIK_V,          0,              4,     6,     0,    0 }, //   V
	{ DIK_W,          0,              4,     7,     0,    0 }, //   W
	{ DIK_X,          0,              8,     0,     0,    0 }, //   X
	{ DIK_Y,          0,              8,     1,     0,    0 }, //   Y
	{ DIK_Z,          0,              8,     2,     0,    0 }, //   Z
	{ DIK_0,          0,             16,     0,     0,    0 }, //   0
	{ DIK_1,          0,             16,     1,     0,    0 }, //   1
	{ DIK_2,          0,             16,     2,     0,    0 }, //   2
	{ DIK_3,          0,             16,     3,     0,    0 }, //   3
	{ DIK_4,          0,             16,     4,     0,    0 }, //   4
	{ DIK_5,          0,             16,     5,     0,    0 }, //   5
	{ DIK_6,          0,             16,     6,     0,    0 }, //   6
	{ DIK_7,          0,             16,     7,     0,    0 }, //   7
	{ DIK_8,          0,             32,     0,     0,    0 }, //   8
	{ DIK_9,          0,             32,     1,     0,    0 }, //   9
	{ DIK_1,          DIK_LSHIFT,    16,     1,    64,    7 }, //   !
	{ DIK_2,          DIK_LSHIFT,     1,     0,     0,    0 }, //   @
	{ DIK_3,          DIK_LSHIFT,    16,     3,    64,    7 }, //   #
	{ DIK_4,          DIK_LSHIFT,    16,     4,    64,    7 }, //   $
	{ DIK_5,          DIK_LSHIFT,    16,     5,	   64,    7 }, //   %
	{ DIK_6,          DIK_LSHIFT,    16,     7,    64,    4 }, //   ^ (CoCo CTRL 7)
	{ DIK_7,          DIK_LSHIFT,    16,     6,    64,    7 }, //   &
	{ DIK_8,          DIK_LSHIFT,    32,     2,    64,    7 }, //   *
	{ DIK_9,          DIK_LSHIFT,    32,     0,    64,    7 }, //   (
	{ DIK_0,          DIK_LSHIFT,    32,     1,    64,    7 }, //   )

	{ DIK_SEMICOLON,  0,             32,     3,     0,    0 }, //   ;
	{ DIK_SEMICOLON,  DIK_LSHIFT,    32,     2,     0,    0 }, //   :

	{ DIK_APOSTROPHE, 0,             16,     7,    64,    7 }, //   '
	{ DIK_APOSTROPHE, DIK_LSHIFT,    16,     2,    64,    7 }, //   "

	{ DIK_COMMA,      0,             32,     4,     0,    0 }, //   ,
	{ DIK_PERIOD,     0,             32,     6,     0,    0 }, //   .
	{ DIK_SLASH,      DIK_LSHIFT,    32,     7,    64,    7 }, //   ?
	{ DIK_SLASH,      0,             32,     7,     0,    0 }, //   /
	{ DIK_EQUALS,     DIK_LSHIFT,    32,     3,    64,    7 }, //   +
	{ DIK_EQUALS,     0,             32,     5,    64,    7 }, //   =
	{ DIK_MINUS,      0,             32,     5,     0,    0 }, //   -
	{ DIK_MINUS,      DIK_LSHIFT,    32,     5,    64,    4 }, //   _ (underscore) (CoCo CTRL -)

															   // added
	{ DIK_UPARROW,    0,              8,     3,     0,    0 }, //   UP
	{ DIK_DOWNARROW,  0,              8,     4,     0,    0 }, //   DOWN
	{ DIK_LEFTARROW,  0,              8,     5,     0,    0 }, //   LEFT
	{ DIK_RIGHTARROW, 0,              8,     6,     0,    0 }, //   RIGHT

	{ DIK_NUMPAD8,    0,              8,     3,     0,    0 }, //   UP
	{ DIK_NUMPAD2,    0,              8,     4,     0,    0 }, //   DOWN
	{ DIK_NUMPAD4,    0,              8,     5,     0,    0 }, //   LEFT
	{ DIK_NUMPAD6,    0,              8,     6,     0,    0 }, //   RIGHT
	{ DIK_SPACE,      0,              8,     7,     0,    0 }, //   SPACE

	{ DIK_RETURN,     0,             64,     0,     0,    0 }, //   ENTER
	{ DIK_NUMPAD7,    0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ DIK_HOME,       0,             64,     1,     0,    0 }, //   HOME (CLEAR)
	{ DIK_ESCAPE,     0,             64,     2,     0,    0 }, //   ESCAPE (BREAK)
	{ DIK_NUMPAD1,    0,             64,     7,     8,    6 }, //   END OF LINE (SHIFT)(RIGHT)
	{ DIK_END,        0,             64,     7,     8,    6 }, //   END OF LINE (SHIFT)(RIGHT)
	{ DIK_NUMPADPERIOD, 0,           64,     4,     8,    5 }, //   DELETE (CTRL)(LEFT)
	{ DIK_NUMPAD0,    0,             64,     4,     8,    6 }, //   INSERT (CTRL)(RIGHT)
	{ DIK_NUMPAD9,    0,             64,     7,     8,    3 }, //   PAGEUP (SHFT)(UP)
	{ DIK_NUMPAD3,    0,             64,     7,     8,    4 }, //   PAGEDOWN (SHFT)(DOWN)
	{ DIK_F1,         0,             64,     5,     0,    0 }, //   F1
	{ DIK_F2,         0,             64,     6,     0,    0 }, //   F2
	{ DIK_BACK,       0,              8,     5,     0,    0 }, //   BACKSPACE -> CoCo left arrow

	{ DIK_LBRACKET,   0,             64,     4,    32,    0 }, //   [ (CoCo CTRL 8)
	{ DIK_RBRACKET,   0,             64,     4,    32,    1 }, //   ] (CoCo CTRL 9)
	{ DIK_LBRACKET,   DIK_LSHIFT,    64,     4,    32,    4 }, //   { (CoCo CTRL ,)
	{ DIK_RBRACKET,   DIK_LSHIFT,    64,     4,    32,    6 }, //   } (CoCo CTRL .)

	{ DIK_BACKSLASH,  0,             32,     7,    64,    4 }, //   '\' (Back slash) (CoCo CTRL /)
	{ DIK_BACKSLASH,  DIK_LSHIFT,    16,     1,    64,    4 }, //   | (Pipe) (CoCo CTRL 1)
	{ DIK_GRAVE,      DIK_LSHIFT,    16,     3,    64,    4 }, //   ~ (tilde) (CoCo CTRL 3)

	{ DIK_CAPSLOCK,   0,             64,     4,    16,    0 }, //   CAPS LOCK (CoCo CTRL 0 for OS-9)
															   //	{ DIK_CAPSLOCK,   0,             64,     7,    16,    0 }, //   CAPS LOCK (CoCo Shift 0 for DECB)

															   //	{ DIK_SCROLL,     0,              1,     0,    64,    7 }, //   SCROLL (CoCo SHIFT @)

	{ DIK_LALT,       0,             64,     3,     0,    0 }, //   ALT
	{ DIK_LCONTROL,   0,             64,     4,     0,    0 }, //   CTRL
	{ DIK_LSHIFT,     0,             64,     7,     0,    0 }, //   SHIFT (the code converts DIK_RSHIFT to DIK_LSHIFT)

	{ 0,              0,              0,     0,     0,    0 }  // terminator
};

