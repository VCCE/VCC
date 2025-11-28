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
#include "multipak_cartridge_driver.h"
#include "vcc/bus/expansion_port_bus.h"


namespace vcc::cartridges::multipak
{

	/// @brief Interface to the expansion port bus.
	///
	/// This Expansion Port Bus extends the base implementation to support routing of bus
	/// related calls made by the cartridges inserted into the Multi-Pak. All calls except
	/// `set_cartridge_select_line` are routed to the bus the Multi-Pak. The calls to
	/// `set_cartridge_select_line` are routed to the Multi-Pak Cartridge Driver.
	class multipak_expansion_port_bus : public ::vcc::bus::expansion_port_bus
	{
	public:

		/// @brief Type alias for the component emulating the Multi-Pak hardware.
		using driver_type = ::vcc::cartridges::multipak::multipak_cartridge_driver;
		/// @copydoc driver_type::slot_id_type
		using slot_id_type = driver_type::slot_id_type;


	public:

		/// @brief Construct the Multi-Pak expansion bus adapter.
		/// 
		/// @param bus A pointer to the expansion bus the Multi-Pak is connected to.
		/// @param slot_id the Id of the slot the adapter is for.
		/// @param driver The Multi-Pak driver.
		/// 
		/// @throws std::invalid_argument if `bus` is null.
		multipak_expansion_port_bus(
			std::shared_ptr<::vcc::bus::expansion_port_bus> bus,
			slot_id_type slot_id,
			driver_type& driver);

		/// @inheritdoc
		void reset() override
		{
			bus_->reset();
		}

		/// @inheritdoc
		void write_memory_byte(unsigned char value, unsigned short address) override
		{
			bus_->write_memory_byte(value, address);
		}

		/// @inheritdoc
		[[nodiscard]] unsigned char read_memory_byte(unsigned short address) override
		{
			return bus_->read_memory_byte(address);
		}

		/// @brief Set the cartridge select line.
		/// 
		/// Set the cartridge select line for the cartridge. Unlike other calls this
		/// is routed to the Multi-Pak driver instead of the bus of the host system.
		/// 
		/// @param line_state The new state of the cartridge select line.
		void set_cartridge_select_line(bool line_state) override
		{
			driver_.set_cartridge_select_line(slot_id_, line_state);
		}

		/// @inheritdoc
		void assert_irq_interrupt_line() override
		{
			bus_->assert_irq_interrupt_line();
		}

		/// @inheritdoc
		void assert_nmi_interrupt_line() override
		{
			bus_->assert_nmi_interrupt_line();
		}

		/// @inheritdoc
		void assert_cartridge_interrupt_line() override
		{
			bus_->assert_cartridge_interrupt_line();
		}


	private:

		/// @brief The parent bus the Multi-Pak is connected to.
		const std::shared_ptr<::vcc::bus::expansion_port_bus> bus_;
		/// @brief The slot id the adapter is for.
		const slot_id_type slot_id_;
		/// @brief The Multi-Pak cartridge driver.
		driver_type& driver_;
	};

}
