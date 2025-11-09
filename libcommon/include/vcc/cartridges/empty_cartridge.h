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
#include <vcc/bus/cartridge.h>


namespace vcc::cartridges
{

	/// @brief A cartridge that does nothing.
	/// 
	/// The Empty Cartridge provides no functionality, extensions, or ROM to the system. It
	/// does nothing and can act as a placeholder where a cartridge instance may be needed.
	class LIBCOMMON_EXPORT empty_cartridge final : public ::vcc::bus::cartridge 
	{
	public:

		/// @brief Retrieves the name of the cartridge.
		/// 
		/// @return An empty string.
		[[nodiscard]] name_type name() const override;

		/// @brief Retrieves an optional catalog identifier of the cartridge.
		/// 
		/// @return An empty string.
		[[nodiscard]] catalog_id_type catalog_id() const override;

		/// @brief Retrieves an optional description of the cartridge.
		/// 
		/// @return An empty string.
		[[nodiscard]] description_type description() const override;
	};

}
