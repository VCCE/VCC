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
#include <vcc/core/cartridge.h>
#include <vcc/core/legacy_cartridge_definitions.h>
#include <vector>


namespace vcc { namespace core { namespace cartridges
{

	class rom_cartridge : public cartridge
	{
	public:

		using buffer_type = std::vector<uint8_t>;
		using size_type = std::size_t;


	public:

		using cartridge::cartridge;


		LIBCOMMON_EXPORT rom_cartridge(
			AssertCartridgeLineModuleCallback assertCartCallback,
			name_type name,
			catalog_id_type catalog_id,
			buffer_type buffer,
			bool enable_bank_switching);
		
		LIBCOMMON_EXPORT const name_type& name() const override;
		LIBCOMMON_EXPORT const catalog_id_type& catalog_id() const override;

		LIBCOMMON_EXPORT void reset() override;
		LIBCOMMON_EXPORT void write_port(unsigned char port_id, unsigned char value) override;
		LIBCOMMON_EXPORT unsigned char read_memory_byte(unsigned short memory_address) override;


	protected:

		LIBCOMMON_EXPORT void initialize_bus() override;


	private:

		const AssertCartridgeLineModuleCallback assertCartCallback_;
		const name_type name_;
		const catalog_id_type catalog_id_;
		const buffer_type buffer_;
		const bool enable_bank_switching_;
		size_type bank_offset_;
	};

} } }
