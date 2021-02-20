
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

/*****************************************************************************/
#ifndef _keyboardedit_h_
#define _keyboardedit_h_
/*****************************************************************************/

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "resource.h"

#ifdef __cplusplus
extern "C"
{
#endif

int LoadCustomKeyMap(char* keymapfile);
BOOL CALLBACK KeyMapProc(HWND, UINT, WPARAM, LPARAM);

#ifdef __cplusplus
}
#endif

/*****************************************************************************/
#endif  // _keyboardedit_h_
/*****************************************************************************/
