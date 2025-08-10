#pragma once

//  Copyright 2015 by Joseph Forgion
//  This file is part of VCC (Virtual Color Computer).
//
//  VCC (Virtual Color Computer) is free software: you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  VCC (Virtual Color Computer) is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with VCC. If not, see <http://www.gnu.org/licenses/>.
//
//  2025/05/22 - Craig Allsop - Interrupt handling.
//

// no includes here, included by 6809.c, 6309.c

//
// Access cc flags as boolean
//
static inline bool CC(char flag) 
{
	return cc[flag] ? true : false; 
}

//
// Interrupt lines reading from output of the latch
//
static inline bool FIRQ() { return InterruptLatch.Q & Bit(INT_FIRQ) ? true : false; }
static inline bool IRQ() { return InterruptLatch.Q & Bit(INT_IRQ) ? true : false; }
static inline bool NMI() { return InterruptLatch.Q & Bit(INT_NMI) ? true : false; }

//
// Latch the interrupt lines
// This done per instruction top of state A (see figure 15, 6809 spec) and also waiting in SYNC
//
static inline void LatchInterrupts()
{
	uint8_t OR = 0;
	for (int is = 0; is < IS_MAX; ++is)
		OR |= InterruptLine[is];

	// clock next input
	InterruptLatch.Clock(OR);
}

//
// Clear the nmi interrupt
//
static inline void ClearNMI()
{
	InterruptLine[IS_NMI] &= BitMask(INT_NMI);
	LatchInterrupts();
}

//
// Clear the irq interrupt if source IS_IRQ
//
static inline void ClearIRQ()
{
	InterruptLine[IS_IRQ] &= BitMask(INT_IRQ);
	LatchInterrupts();
}

//
// Clear state of all interrupt lines
//
static inline void ClearInterrupts()
{
	for (int is = 0; is < IS_MAX; ++is)
		InterruptLine[is] = 0;
	InterruptLatch.Reset();
}

