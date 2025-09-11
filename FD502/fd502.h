#ifndef __FD502_H__
#define __FD502_H__
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

// Comment next line to exclude becker code
#define COMBINE_BECKER

extern "C" void (*AssertInt)(unsigned char,unsigned char);
void BuildDynaMenu(void);
// FIXME: These need to be turned into an enum and the signature of functions
// that use them updated. These are also duplicated everywhere and need to be
// consolidated in one gdmf place.
#define	HEAD 0
#define SLAVE 1
#define STANDALONE 2

#define External 0
#define TandyDisk 1
#define RGBDisk 2

#endif
