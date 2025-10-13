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
#include "host_cartridge_context.h"
#include <vcc/core/cartridge.h>
#include <vcc/core/cartridge_loader.h>
#include "../CartridgeMenu.h"


namespace vcc { namespace modules { namespace mpi
{

	class cartridge_slot
	{
	public:

		using name_type = ::vcc::core::cartridge::name_type;
		using catalog_id_type = ::vcc::core::cartridge::catalog_id_type;
		using path_type = ::std::string;
		using label_type = ::std::string;
		using handle_type = ::vcc::core::cartridge_loader_result::handle_type;
		using cartridge_ptr_type = ::std::unique_ptr<::vcc::core::cartridge>;
		using menu_item_type = CartMenuItem;
		using menu_item_collection_type = std::vector<menu_item_type>;
		using context_type = host_cartridge_context;// ::vcc::core::cartridge_context;


	public:

		cartridge_slot();
		cartridge_slot(path_type path, handle_type handle, cartridge_ptr_type cartridge);
		cartridge_slot(const cartridge_slot&) = delete;
		cartridge_slot(cartridge_slot&&) = delete;

		cartridge_slot& operator=(const cartridge_slot& other) = delete;
		cartridge_slot& operator=(cartridge_slot&& other) noexcept;

		bool empty() const
		{
			return path_.empty();
		}

		const path_type& path() const
		{
			return path_;
		}

		name_type name() const
		{
			return cartridge_->name();
		}

		catalog_id_type catalog_id() const
		{
			return cartridge_->catalog_id();
		}

		label_type label() const;

		void start() const
		{
			cartridge_->start();
		}

		void reset() const
		{
			cartridge_->reset();
		}

		void heartbeat() const
		{
			cartridge_->heartbeat();
		}

		void write_port(unsigned char port_id, unsigned char value) const
		{
			cartridge_->write_port(port_id, value);
		}

		unsigned char read_port(unsigned char port_id) const
		{
			return cartridge_->read_port(port_id);
		}

		unsigned char read_memory_byte(unsigned short memory_address) const
		{
			return cartridge_->read_memory_byte(memory_address);
		}

		void status(char* status) const
		{
			cartridge_->status(status);
		}

		unsigned short sample_audio() const
		{
			return cartridge_->sample_audio();
		}

		void menu_item_clicked(unsigned char item_id) const
		{
			return cartridge_->menu_item_clicked(item_id);
		}

		void line_state(bool state)
		{
			line_state_ = state;
		}

		bool line_state() const
		{
			return line_state_;
		}

		void reset_menu()
		{
			menu_items_.clear();
		}

		void append_menu_item(const menu_item_type& item)
		{
			menu_items_.push_back(item);
		}

		void enumerate_menu_items(context_type& context) const
		{
			for (const auto& item : menu_items_)
			{
				context.add_menu_item(item.name.c_str(), item.menu_id, item.type);
			}
		}


	private:

		path_type path_;
		handle_type handle_;
		cartridge_ptr_type cartridge_;
		menu_item_collection_type menu_items_;
		bool line_state_ = false;
	};

} } }
