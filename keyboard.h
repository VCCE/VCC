#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__
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

void KeyboardEvent( unsigned char, unsigned char,unsigned int);
unsigned char kb_scan(unsigned char);
void SetKeyboardInteruptState(unsigned char);
//void KeyboardInit(void);

void joystick (short unsigned,short unsigned);
unsigned short get_pot_value(unsigned char pot);
void SetButtonStatus(unsigned char,unsigned char);
void SetStickNumbers(unsigned char,unsigned char);
//int InitJoyStick(void);

#endif
