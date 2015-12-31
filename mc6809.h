#ifndef __MC6809_H__
#define __MC6809_H__

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

void MC6809Init(void);
int  MC6809Exec( int);
void MC6809Reset(void);
void MC6809AssertInterupt(unsigned char,unsigned char);
void MC6809DeAssertInterupt(unsigned char);// 4 nmi 2 firq 1 irq
void MC6809ForcePC(unsigned short);

#endif
