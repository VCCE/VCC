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
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_bus.h"
#include "vcc/utils/borrowed_ptr.h"
#include "vcc/utils/cartridge_loader.h"
#include "vcc/detail/exports.h"
#include <memory>


namespace vcc::bus
{

	class expansion_port
	{
	public:

		using size_type = std::size_t;
		using managed_handle_type = ::vcc::utils::cartridge_loader_result::handle_type;
		using cartridge_type = ::vcc::bus::cartridge;
		using cartridge_ptr_type = std::shared_ptr<cartridge_type>;
		using device_type = ::vcc::bus::cartridge_device;
		using device_ptr_type = ::vcc::utils::borrowed_ptr<device_type*>;
		using name_type = cartridge_type::name_type;
		using catalog_id_type = cartridge_type::catalog_id_type;
		using description_type = cartridge_type::description_type;
		using menu_item_collection_type = cartridge_type::menu_item_collection_type;

	public:

		LIBCOMMON_EXPORT expansion_port();
		LIBCOMMON_EXPORT expansion_port(managed_handle_type handle, cartridge_ptr_type cartridge);
		expansion_port(const expansion_port&) = delete;
		LIBCOMMON_EXPORT expansion_port(expansion_port&&) = default;

		LIBCOMMON_EXPORT virtual ~expansion_port() = default;

		expansion_port& operator=(const expansion_port& other) = delete;
		LIBCOMMON_EXPORT expansion_port& operator=(expansion_port&& other) noexcept = default;

		[[nodiscard]] bool empty() const
		{
			return cartridge_ == nullptr;
		}

		[[nodiscard]] name_type name() const
		{
			return cartridge_->name();
		}

		[[nodiscard]] catalog_id_type catalog_id() const
		{
			return cartridge_->catalog_id();
		}

		[[nodiscard]] description_type description() const
		{
			return cartridge_->description();
		}

		void start() const
		{
			cartridge_->start();
		}

		void stop() const
		{
			cartridge_->stop();
		}

		void status(char* text_buffer, size_t buffer_size) const
		{
			cartridge_->status(text_buffer, buffer_size);
		}

		[[nodiscard]] menu_item_collection_type get_menu_items() const
		{
			return cartridge_->get_menu_items();
		}

		void menu_item_clicked(unsigned char item_id) const
		{
			return cartridge_->menu_item_clicked(item_id);
		}

		void reset() const
		{
			device_->reset();
		}

		void update(float delta) const
		{
			device_->update(delta);
		}

		void write_port(unsigned char port_id, unsigned char value) const
		{
			device_->write_port(port_id, value);
		}

		[[nodiscard]] unsigned char read_port(unsigned char port_id) const
		{
			return device_->read_port(port_id);
		}

		[[nodiscard]] unsigned char read_memory_byte(size_type memory_address) const
		{
			return device_->read_memory_byte(memory_address);
		}

		[[nodiscard]] unsigned short sample_audio() const
		{
			return device_->sample_audio();
		}


	private:

		cartridge_ptr_type cartridge_;
		managed_handle_type handle_;
		device_ptr_type device_;
	};

}
