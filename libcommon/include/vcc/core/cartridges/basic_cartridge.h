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


namespace vcc { namespace core { namespace cartridges
{

	struct LIBCOMMON_EXPORT basic_cartridge : public ::vcc::core::cartridge
	{
	public:

		using name_type = std::string;
		using catalog_id_type = std::string;


	public:

		using cartridge::cartridge;

		name_type name() const override;
		catalog_id_type catalog_id() const override;
		description_type description() const override;

		void start() override;
		void stop() override;

		void reset() override;
		void process_horizontal_sync() override;
		void write_port(unsigned char port_id, unsigned char value) override;
		unsigned char read_port(unsigned char port_id) override;
		unsigned char read_memory_byte(unsigned short memory_address) override;
		void status(char* text_buffer, size_t buffer_size) override;
		unsigned short sample_audio() override;
		void menu_item_clicked(unsigned char menu_item_id) override;


	protected:

		virtual void initialize_pak();
		virtual void initialize_bus();
	};

} } }
