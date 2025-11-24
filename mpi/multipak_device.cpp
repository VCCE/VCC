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
#include "multipak_device.h"
#include "multipak_expansion_port_ui.h"
#include "multipak_expansion_port_bus.h"
#include "resource.h"


namespace
{

	constexpr std::pair<size_t, size_t> disk_controller_io_port_range = { 0x40, 0x5f };

	// Is a port a disk port?
	constexpr bool is_disk_controller_port(multipak_device::slot_id_type port)
	{
		return port >= disk_controller_io_port_range.first && port <= disk_controller_io_port_range.second;
	}

}


multipak_device::multipak_device(
	std::shared_ptr<expansion_port_bus_type> bus,
	const multipak_configuration& configuration)
	:
	bus_(move(bus)),
	configuration_(configuration)
{ }


void multipak_device::start()
{
	// Get the startup slot and set Chip select and SCS slots from ini file
	switch_slot_ = configuration_.selected_slot();
	cached_cts_slot_ = switch_slot_;
	cached_scs_slot_ = switch_slot_;
}


void multipak_device::stop()
{
	for (auto slot(0u); slot < slots_.size(); slot++)
	{
		eject_cartridge(slot);
	}
}

void multipak_device::reset()
{
	vcc::utils::section_locker lock(mutex_);

	cached_cts_slot_ = switch_slot_;
	cached_scs_slot_ = switch_slot_;
	for (const auto& cartridge_slot : slots_)
	{
		cartridge_slot.reset();
	}

	bus_->set_cartridge_select_line(slots_[cached_scs_slot_].line_state());
}

void multipak_device::update(float delta)
{
	vcc::utils::section_locker lock(mutex_);

	for(const auto& cartridge_slot : slots_)
	{
		cartridge_slot.update(delta);
	}
}

void multipak_device::write_port(unsigned char port_id, unsigned char value)
{
	vcc::utils::section_locker lock(mutex_);

	if (port_id == slot_select_port_id)
	{
		cached_scs_slot_ = value & 0b00000011;			// SCS
		cached_cts_slot_ = (value >> 4) & 0b00000011;	// CTS
		slot_register_ = value;

		bus_->set_cartridge_select_line(slots_[cached_scs_slot_].line_state());

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

unsigned char multipak_device::read_port(unsigned char port_id)
{
	vcc::utils::section_locker lock(mutex_);

	if (port_id == slot_select_port_id)	// Self
	{
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

unsigned char multipak_device::read_memory_byte(size_type memory_address)
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[cached_cts_slot_].read_memory_byte(memory_address);
}

unsigned short multipak_device::sample_audio()
{
	vcc::utils::section_locker lock(mutex_);

	unsigned short sample = 0;
	for (const auto& cartridge_slot : slots_)
	{
		sample += cartridge_slot.sample_audio();
	}

	return sample;
}


multipak_device::name_type multipak_device::slot_name(slot_id_type slot) const
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[slot].name();
}

bool multipak_device::empty(slot_id_type slot) const
{
	vcc::utils::section_locker lock(mutex_);

	return slots_[slot].empty();
}

void multipak_device::eject_cartridge(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	slots_[slot].stop();
	slots_[slot] = {};
}


void multipak_device::switch_to_slot(slot_id_type slot)
{
	vcc::utils::section_locker lock(mutex_);

	switch_slot_ = slot;
	cached_scs_slot_ = slot;
	cached_cts_slot_ = slot;

	bus_->set_cartridge_select_line(slots_[slot].line_state());
}

multipak_device::slot_id_type multipak_device::selected_switch_slot() const
{
	return switch_slot_;
}

multipak_device::slot_id_type multipak_device::selected_scs_slot() const
{
	return cached_scs_slot_;
}

multipak_device::slot_id_type multipak_device::selected_cts_slot() const
{
	return cached_cts_slot_;
}


void multipak_device::set_cartridge_select_line(slot_id_type slot, bool line_state)
{
	vcc::utils::section_locker lock(mutex_);

	slots_[slot].line_state(line_state);
	if (selected_scs_slot() == slot)
	{
		bus_->set_cartridge_select_line(slots_[slot].line_state());
	}
}


void multipak_device::insert_cartridge(
	slot_id_type slot,
	handle_type handle,
	cartridge_ptr_type cartridge)
{
	// FIXME-CHET: We should probably call eject(slot) here in order to ensure that the
	// cartridge is shut down correctly.
	vcc::utils::section_locker lock(mutex_);

	slots_[slot] = { move(handle), move(cartridge) };
	slots_[slot].start();
	slots_[slot].reset();	//	FIXME-CHET: This is legacy shit and doesn't need to be here. remove after all carts get full refactor
}
