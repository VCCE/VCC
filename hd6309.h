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

void HD6309Init();
int  HD6309Exec( int);
void HD6309Reset();
void HD6309AssertInterupt(InterruptSource, Interrupt);
void HD6309DeAssertInterupt(InterruptSource, Interrupt);
void HD6309ForcePC(unsigned short);
VCC::CPUState HD6309GetState();
void HD6309SetBreakpoints(const std::vector<unsigned short>& breakpoints);
void HD6309SetTraceTriggers(const std::vector<unsigned short>& triggers);

#endif
