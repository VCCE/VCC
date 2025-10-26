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
#include <vcc/core/cartridge_context.h>
#include <memory>
#include <Windows.h>


class orchestra90cc_cartridge : public ::vcc::core::cartridge
{
public:
	
	using context_type = ::vcc::core::cartridge_context;

	orchestra90cc_cartridge(HINSTANCE module_instance, std::unique_ptr<context_type> context);

	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;

	void reset() override;

	unsigned char read_memory_byte(unsigned short memory_address) override;

	void write_port(unsigned char port_id, unsigned char value) override;

	unsigned short sample_audio() override;

private:

	const HINSTANCE module_instance_;
	const std::unique_ptr<context_type> context_;
	unsigned char left_channel_buffer_ = 0;
	unsigned char right_channel_buffer_ = 0;
};
