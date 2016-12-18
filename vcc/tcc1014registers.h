#pragma once
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


void GimeWrite(unsigned char,unsigned char);
unsigned char GimeRead(unsigned char);
void GimeAssertKeyboardInterupt();
unsigned char GimeGetKeyboardInteruptState();
void GimeAssertHorzInterupt();
void GimeAssertVertInterupt();
void GimeAssertTimerInterupt();
unsigned char sam_read(unsigned char);
void sam_write(unsigned char,unsigned char);
void mc6883_reset();
unsigned char VDG_Offset();
unsigned char VDG_Modes();
