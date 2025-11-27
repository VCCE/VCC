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
#include "multipak_cartridge_driver.h"
#include "multipak_expansion_port_ui.h"
#include "multipak_expansion_port_bus.h"
#include "resource.h"
#include <stdexcept>


namespace vcc::cartridges::multipak
{

	namespace
	{

		constexpr std::pair<size_t, size_t> disk_controller_io_port_range = { 0x40, 0x5f };

		// Is a port a disk port?
		constexpr bool is_disk_controller_port(multipak_cartridge_driver::slot_id_type port)
		{
			return port >= disk_controller_io_port_range.first && port <= disk_controller_io_port_range.second;
		}

	}


	multipak_cartridge_driver::multipak_cartridge_driver(
		std::shared_ptr<expansion_port_bus_type> bus,
		std::shared_ptr<multipak_configuration> configuration)
		:
		bus_(move(bus)),
		configuration_(move(configuration))
	{
		if (bus_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge Driver. Bus is null.");
		}

		if (configuration_ == nullptr)
		{
			throw std::invalid_argument("Cannot construct Multi-Pak Cartridge Driver. Configuration is null.");
		}
	}


	void multipak_cartridge_driver::start()
	{
		switch_slot_ = configuration_->selected_slot();
		cached_cts_slot_ = switch_slot_;
		cached_scs_slot_ = switch_slot_;
	}


	void multipak_cartridge_driver::stop()
	{
		for (auto slot(0u); slot < slots_.size(); slot++)
		{
			eject_cartridge(slot);
		}
	}

	void multipak_cartridge_driver::reset()
	{
		cached_cts_slot_ = switch_slot_;
		cached_scs_slot_ = switch_slot_;
		for (const auto& cartridge_slot : slots_)
		{
			cartridge_slot.reset();
		}

		bus_->set_cartridge_select_line(slots_[cached_scs_slot_].cartridge_select_line());
	}

	void multipak_cartridge_driver::update(float delta)
	{
		for (const auto& cartridge_slot : slots_)
		{
			cartridge_slot.update(delta);
		}
	}

	void multipak_cartridge_driver::write_port(unsigned char port_id, unsigned char value)
	{
		if (port_id == mmio_ports::slot_select)
		{
			cached_scs_slot_ = value & 0b00000011;			// SCS
			cached_cts_slot_ = (value >> 4) & 0b00000011;	// CTS
			slot_register_ = value;

			bus_->set_cartridge_select_line(slots_[cached_scs_slot_].cartridge_select_line());

			return;
		}

		// Only write disk ports (0x40-0x5F) if SCS is set
		if (is_disk_controller_port(port_id))
		{
			slots_[cached_scs_slot_].write_port(port_id, value);
			return;
		}

		for (const auto& cartridge_slot : slots_)
		{
			cartridge_slot.write_port(port_id, value);
		}
	}

	unsigned char multipak_cartridge_driver::read_port(unsigned char port_id)
	{
		if (port_id == mmio_ports::slot_select)
		{
			// FIXME-CHET: The next two lines should be handled by the write_port function
			slot_register_ &= 0b11001100;
			slot_register_ |= cached_scs_slot_ | (cached_cts_slot_ << 4);

			return slot_register_;
		}

		// Only read disk ports (0x40-0x5F) if SCS is set
		if (is_disk_controller_port(port_id))
		{
			return slots_[cached_scs_slot_].read_port(port_id);
		}

		for (const auto& cartridge_slot : slots_)
		{
			// Return value from first module that returns non zero
			// FIXME-CHET: Shouldn't this OR all the values together?
			const auto data(cartridge_slot.read_port(port_id));
			if (data != 0)
			{
				return data;
			}
		}

		return 0;
	}

	unsigned char multipak_cartridge_driver::read_memory_byte(size_type memory_address)
	{
		return slots_[cached_cts_slot_].read_memory_byte(memory_address);
	}

	multipak_cartridge_driver::sample_type multipak_cartridge_driver::sample_audio()
	{
		sample_type sample = 0;
		for (const auto& cartridge_slot : slots_)
		{
			sample += cartridge_slot.sample_audio();
		}

		return sample;
	}


	multipak_cartridge_driver::name_type multipak_cartridge_driver::slot_name(slot_id_type slot) const
	{
		return slots_[slot].name();
	}

	bool multipak_cartridge_driver::empty(slot_id_type slot) const
	{
		return slots_[slot].empty();
	}

	void multipak_cartridge_driver::eject_cartridge(slot_id_type slot)
	{
		slots_[slot].stop();
		slots_[slot] = {};
	}


	void multipak_cartridge_driver::switch_to_slot(slot_id_type slot)
	{
		switch_slot_ = slot;
		// FIXME-CHET: Should this set the scs and cts lines when the selection is physically
		// changed or should they only be loaded at startup?
		cached_scs_slot_ = slot;
		cached_cts_slot_ = slot;

		bus_->set_cartridge_select_line(slots_[slot].cartridge_select_line());
	}

	multipak_cartridge_driver::slot_id_type multipak_cartridge_driver::selected_switch_slot() const
	{
		return switch_slot_;
	}

	multipak_cartridge_driver::slot_id_type multipak_cartridge_driver::selected_scs_slot() const
	{
		return cached_scs_slot_;
	}

	multipak_cartridge_driver::slot_id_type multipak_cartridge_driver::selected_cts_slot() const
	{
		return cached_cts_slot_;
	}


	void multipak_cartridge_driver::set_cartridge_select_line(slot_id_type slot, bool line_state)
	{
		slots_[slot].cartridge_select_line(line_state);
		if (selected_scs_slot() == slot)
		{
			bus_->set_cartridge_select_line(slots_[slot].cartridge_select_line());
		}
	}


	void multipak_cartridge_driver::insert_cartridge(
		slot_id_type slot,
		managed_handle_type handle,
		cartridge_ptr_type cartridge)
	{
		// FIXME-CHET: We should probably call eject(slot) here in order to ensure that the
		// cartridge is shut down correctly.
		slots_[slot] = { move(handle), move(cartridge) };
		slots_[slot].start();
		slots_[slot].reset();	//	FIXME-CHET: This is legacy shit and doesn't need to be here. remove after all carts get full refactor
	}

}
