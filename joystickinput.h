#ifndef __JOYSTICKINPUT_H__
#define __JOYSTICKINPUT_H__
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

#include <dinput.h>
#include "mc6821.h"
#define MAXSTICKS 10
#define STRLEN 64

typedef struct {
	unsigned char UseMouse;
	unsigned char Up;
	unsigned char Down;
	unsigned char Left;
	unsigned char Right;
	unsigned char Fire1;
	unsigned char Fire2;
	unsigned char DiDevice;
	unsigned char HiRes;
} JoyStick;

HRESULT JoyStickPoll(DIJOYSTATE2 * ,unsigned char);
int EnumerateJoysticks(void);
bool InitJoyStick (unsigned char);

#ifdef __cplusplus
extern "C"
{
#endif
unsigned char vccJoystickGetScan(unsigned char);
unsigned char SetMouseStatus(unsigned char,unsigned char);

// globals referenced from config.c
extern JoyStick	Left;
extern JoyStick Right;

void			joystick(short unsigned, short unsigned);
unsigned short	get_pot_value(unsigned char pot);
void			SetButtonStatus(unsigned char, unsigned char);
void			SetStickNumbers(unsigned char, unsigned char);

#ifdef __cplusplus
}
#endif

#endif
