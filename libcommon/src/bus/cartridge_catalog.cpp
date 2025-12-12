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
#include <vcc/bus/cartridge_catalog.h>
#include <Windows.h>
#include <ranges>


namespace vcc::bus
{

	namespace
	{

		const ::vcc::bus::cartridge_catalog::item_map_type cartridge_items_;

	}

	cartridge_catalog::cartridge_catalog(path_type cartridge_path)
		:
		cartridge_path_(std::move(cartridge_path)),
		items_(cartridge_items_)
	{
		if (cartridge_path_.empty())
		{
			throw std::invalid_argument("Cannot construct Cartridge Catalog. Cartridge path is empty.");
		}

		if (!std::filesystem::exists(cartridge_path_))
		{
			throw std::invalid_argument("Cannot construct Cartridge Catalog. Cartridge path does not exist or is inaccessible.");
		}
	}


	bool cartridge_catalog::empty() const
	{
		return items_.empty();
	}

	bool cartridge_catalog::is_valid_cartridge_id(const guid_type& id) const
	{
		return items_.contains(id);
	}

	bool cartridge_catalog::is_loaded(const guid_type& id) const
	{
		const auto item_ptr(items_.find(id));
		if (item_ptr == items_.end())
		{
			// FIXME-CHET: This should probably be a different exception or return `false`
			throw std::invalid_argument("Cannot check if cartridge is loaded. Identifier does not exist.");
		}

		return GetModuleHandle(path_type(cartridge_path_).append(item_ptr->second.filename).string().c_str()) != nullptr;
	}

	cartridge_catalog::path_type cartridge_catalog::get_item_pathname(const guid_type& id) const
	{
		const auto item_ptr(items_.find(id));
		if (item_ptr == items_.end())
		{
			// FIXME-CHET: This should probably be a different exception or return std::optional
			throw std::invalid_argument("Cannot get catalog item for cartridge. Identifier does not exist.");
		}

		return path_type(cartridge_path_).append(item_ptr->second.filename);
	}

	cartridge_catalog::item_map_type cartridge_catalog::copy_items() const
	{
		return items_;
	}

	cartridge_catalog::item_container_type cartridge_catalog:: copy_items_ordered() const
	{
		auto values = std::views::values(items_);

		return item_container_type(values.begin(), values.end());
	}

	void cartridge_catalog::accept(const visitor_function_type& visitor) const
	{
		for (const auto& [id, item] : items_)
		{
			visitor(id, item);
		}
	}

}
