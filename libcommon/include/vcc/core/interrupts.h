////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#pragma once

// Enumerations for interrupts and their sources

// CPU interrupts (counting from 1 because legacy)
// Pakinterface uses INT_NONE to clear cart IRQ
enum Interrupt {
	INT_IRQ = 1,
	INT_FIRQ,
	INT_NMI,
	INT_CART
};

// Interrupt sources keep track of their own state.
// NMI is its own source and always uses this.
enum InterruptSource {
	IS_NMI = 0,
	IS_PIA0_HSYNC,
	IS_PIA0_VSYNC,
	IS_PIA1_CD,
	IS_PIA1_CART,
	IS_GIME,
	IS_MAX
};

