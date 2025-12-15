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
		using guid_type = ::vcc::bus::cartridge_catalog::guid_type;

		const auto mpi_catalog_id			= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0x01, 0, 0, 0, 0, 1 });
		const auto fd502_catalog_id			= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0x02, 0, 0, 0, 0, 1 });
		//const auto harddisk_catalog_id	= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0x03, 0, 0, 0, 0, 1 });
		//const auto superide_catalog_id	= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0x03, 0, 0, 0, 0, 2 });
		//const auto rs232pak_catalog_id	= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0x04, 0, 0, 0, 0, 1 });
		const auto becker_port_catalog_id	= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0x04, 0, 0, 0, 0, 2 });
		const auto orch90_catalog_id		= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0x05, 0, 0, 0, 0, 1 });
		const auto gmc_catalog_id			= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0x05, 0, 0, 0, 0, 2 });
		const auto ramdisk_catalog_id		= guid_type({ 0xDE, 0xAD, 0xBE, 0xEF, 0x11, 0x38, 0x19, 0x85, 0x13, 0x37, 0xff, 0, 0, 0, 0, 1 });

		const ::vcc::bus::cartridge_catalog::item_map_type cartridge_items_ = {
			{ mpi_catalog_id,			{ mpi_catalog_id,			"Multi-Pak Interface", "mpi.dll"} },
			{ fd502_catalog_id,			{ fd502_catalog_id,			"FD502 Floppy Disk Controller", "fd502.dll"} },
			//{ harddisk_catalog_id,	{ harddisk_catalog_id,		"VCC Virtual Hard Disk Controller", "", ""} },
			//{ superide_catalog_id,	{ superide_catalog_id,		"IDE Hard Disk Controller", "", ""} },
			//{ rs232pak_catalog_id,	{ rs232pak_catalog_id,		"RS232 Pak", "", ""} },
			{ becker_port_catalog_id,	{ becker_port_catalog_id,	"Becker Port", "becker.dll"} },
			{ gmc_catalog_id,			{ gmc_catalog_id,			"Game Master Cartridge", "gmc.dll"} },
			{ orch90_catalog_id,		{ orch90_catalog_id,		"Orchestra-90cc", "orch90.dll"} },
			{ ramdisk_catalog_id,		{ ramdisk_catalog_id,		"<TODO> Ram Disk", "ramdisk.dll"} },
		};

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
