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
#include <vcc/core/cartridges/rom_cartridge.h>


namespace vcc { namespace core { namespace cartridges
{

	rom_cartridge::rom_cartridge(
		std::unique_ptr<context_type> context,
		name_type name,
		catalog_id_type catalog_id,
		buffer_type buffer,
		bool enable_bank_switching)
		:
		context_(move(context)),
		name_(move(name)),
		catalog_id_(move(catalog_id)),
		buffer_(move(buffer)),
		enable_bank_switching_(enable_bank_switching),
		bank_offset_(0)
	{}


	rom_cartridge::name_type rom_cartridge::name() const
	{
		return name_;
	}

	rom_cartridge::catalog_id_type rom_cartridge::catalog_id() const
	{
		return catalog_id_;
	}


	void rom_cartridge::reset()
	{
		bank_offset_ = 0;
	}

	void rom_cartridge::write_port(unsigned char port_id, unsigned char value)
	{
		if (enable_bank_switching_ && port_id == 0x40)
		{
			bank_offset_ = (value & 0x0f) << 14;
		}
	}

	unsigned char rom_cartridge::read_memory_byte(unsigned short memory_address)
	{
		return buffer_[((memory_address & 32767) + bank_offset_) % buffer_.size()];
	}

	void rom_cartridge::initialize_bus()
	{
		context_->assert_cartridge_line(true);
	}

} } }
