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
#include "multipak_device.h"
#include "vcc/bus/expansion_port_bus.h"


class multipak_expansion_port_bus : public ::vcc::bus::expansion_port_bus
{
public:

	multipak_expansion_port_bus(
		size_t slot_id,
		std::shared_ptr<::vcc::bus::expansion_port_bus> bus,
		//	FIXME: maybe change to shared_ptr (or weak_ptr since its lifetime will be managed by the cartridge)
		multipak_device& device)
		:
		slot_id_(slot_id),
		bus_(bus),
		device_(device)
	{}

	void reset() override
	{
		bus_->reset();
	}

	void write_memory_byte(unsigned char value, unsigned short address) override
	{
		bus_->write_memory_byte(value, address);
	}

	unsigned char read_memory_byte(unsigned short address) override
	{
		return bus_->read_memory_byte(address);
	}

	void set_cartridge_select_line(bool line_state) override
	{
		device_.set_cartridge_select_line(slot_id_, line_state);
	}

	void assert_irq_interrupt_line() override
	{
		bus_->assert_irq_interrupt_line();
	}

	void assert_nmi_interrupt_line() override
	{
		bus_->assert_nmi_interrupt_line();
	}

	void assert_cartridge_interrupt_line() override
	{
		bus_->assert_cartridge_interrupt_line();
	}


private:

	const size_t slot_id_;
	const std::shared_ptr<::vcc::bus::expansion_port_bus> bus_;
	multipak_device& device_;
};
