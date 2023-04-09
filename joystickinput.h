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

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "mc6821.h"
#define MAXSTICKS 10
#define STRLEN 64

// JoyStick structure is used to contain joystick configuration
typedef struct {
	unsigned char UseMouse;  // 0=keyboard,1=mouse,2=audio,3=joystick
	unsigned char Up;
	unsigned char Down;
	unsigned char Left;
	unsigned char Right;
	unsigned char Fire1;
	unsigned char Fire2;
	unsigned char DiDevice;
	unsigned char HiRes;     // 0=lowres,1=software,2=tandy,3=ccmax
} JoyStick;

// Global joystick configuration from config.c  These were
// renamed from Left and Right to preserve maintainer sanity.
extern JoyStick	LeftJS;
extern JoyStick RightJS;

HRESULT JoyStickPoll(DIJOYSTATE2 * ,unsigned char);
int EnumerateJoysticks(void);
bool InitJoyStick (unsigned char);

#ifdef __cplusplus
extern "C"
{
#endif

unsigned char vccJoystickGetScan(unsigned char);
unsigned char SetMouseStatus(unsigned char,unsigned char);
void joystick(unsigned int, unsigned int);
void SetButtonStatus(unsigned char, unsigned char);
void SetStickNumbers(unsigned char, unsigned char);
void vccJoystickStartTandy(unsigned char, unsigned char);
void vccJoystickStartCCMax();

#ifdef __cplusplus
}
#endif

#endif
