#ifndef __HD6309_H__
#define __HD6309_H__
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

void HD6309Init(void);
int  HD6309Exec( int);
void HD6309Reset(void);
void HD6309AssertInterupt(unsigned char,unsigned char);
void HD6309DeAssertInterupt(unsigned char);// 4 nmi 2 firq 1 irq
void HD6309ForcePC(unsigned short);
VCC::CPUState HD6309GetState();
void HD6309SetBreakpoints(const std::vector<unsigned short>& breakpoints);
void HD6309SetTraceTriggers(const std::vector<unsigned short>& triggers);
//unsigned short GetPC(void);

void HD6309Init_s(void);
int  HD6309Exec_s( int);
void HD6309Reset_s(void);
void HD6309AssertInterupt_s(unsigned char,unsigned char);
void HD6309DeAssertInterupt_s(unsigned char);// 4 nmi 2 firq 1 irq
void HD6309ForcePC_s(unsigned short);

#endif
