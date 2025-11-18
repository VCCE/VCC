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
#include "vcc/detail/exports.h"
#include "vcc/bus/interrupts.h"


namespace vcc::bus
{

	/// @brief Interface to the expansion port bus.
	///
	/// The Expansion Port Bus abstract class defines the core bus control functionality
	/// available to cartridges connected to the expansion bus. It's primary purpose is to
	/// allow the cartridge to communicate hardware related events required for property
	/// emulation of the system.
	class LIBCOMMON_EXPORT expansion_port_bus
	{
	public:

		virtual ~expansion_port_bus() = default;

		/// @brief Requests the emulated system be reset.
		///
		/// Informs the system that it should perform a hard reset of the emulated machine.
		virtual void reset() = 0;

		/// @brief Set the assert state of the cartridge line.
		/// 
		/// Set the status of the cartridge select line on the simulated hardware bus.
		/// 
		/// @todo rename to set_cartridge_select or set_cartridge_select_signal.
		/// 
		/// @param line_state The new state of the cartridge line.
		virtual void assert_cartridge_line(bool line_state) = 0;

		/// @brief Issue an interrupt.
		/// 
		/// Assets a selected interrupt line on the simulated hardware bus.
		/// 
		/// @todo Remove interrupt_source as it seems to be irrelevant to the assertion here.
		/// @todo Limit the interrupts that can be asserted to only those that are actually possible.
		/// 
		/// @param interrupt The type of interrupt to asset.
		/// @param interrupt_source The event source that caused the interrupt to be asserted.
		virtual void assert_interrupt(Interrupt interrupt, InterruptSource interrupt_source) = 0;

		/// @brief Write a byte to the CPU address space.
		/// 
		/// @todo Remove this. This is used for DMA and the CoCo 3 does not support it without
		/// modification.
		/// 
		/// @param value The value to write.
		/// @param address The address to write the value to.
		virtual void write_memory_byte(unsigned char value, unsigned short address) = 0;

		/// @brief Read a byte from the CPU address space.
		/// 
		/// @todo Remove this. This is used for DMA and the CoCo 3 does not support it without
		/// modification.
		/// 
		/// @param address The address to read the value from.
		/// 
		/// @return The value at the specified address.
		virtual [[nodiscard]] unsigned char read_memory_byte(unsigned short address) = 0;
	};

}
