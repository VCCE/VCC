#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

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
*/
/*****************************************************************************/
#include <string>


typedef enum keyevent_e
{
	kEventKeyUp		= 0,
	kEventKeyDown	= 1
} keyevent_e;

/**
	Keyboard layouts
*/
typedef enum keyboardlayout_e
{
	kKBLayoutCoCo = 0,
	kKBLayoutNatural,
	kKBLayoutCompact,
	kKBLayoutCustom,

	kKBLayoutCount
} keyboardlayout_e;

/**
	Keyboard layout names used to populate the
	layout selection pull-down in the config dialog

	This of course must match keyboardlayout_e above
*/
const char * const k_keyboardLayoutNames[] =
{
	"CoCo (DECB)",
	"Natural (OS-9)",
	"Compact (OS-9)",
	"Custom"
} ;

/**
*/
typedef struct keytranslationentry_t
{
	unsigned char ScanCode1;
	unsigned char ScanCode2;
	unsigned char Row1;
	unsigned char Col1;
	unsigned char Row2;
	unsigned char Col2;
} keytranslationentry_t;

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	void			vccKeyboardBuildRuntimeTable(keyboardlayout_e keyBoardLayout);
	void			vccKeyboardHandleKey(unsigned char, unsigned char, keyevent_e keyState);
	unsigned char	vccKeyboardGetScan(unsigned char);
#ifdef __cplusplus
}
#endif

/*****************************************************************************/

#endif // __KEYBOARD_H__

bool GetPaste();
void SetPaste(bool);
void PasteIntoQueue(std::string txt);
bool IsShiftKeyDown();
