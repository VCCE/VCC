////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute it and/or
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
/// @file
/// 
/// @brief Defines Disk BASIC ROM identifiers.


namespace vcc::cartridges::fd502::detail
{

	/// @brief Defines identifiers for Disk BASIC ROM images.
	/// 
	/// Defines unique identifiers for different versions of known Disk BASIC ROM
	/// images that can be used by the FD502.
	/// 
	/// @todo add ADOS 3 with a couple of variants based on ADOS settings
	enum class disk_basic_rom_image_id
	{
		/// @brief Identifies a custom ROM image file chosen by the user.
		custom,
		/// @brief The default Microsoft Disk BASIC ROM (i.e. RSDOS)
		microsoft,
		/// @brief RGB DOS (modified for the emulated host?)
		rgbdos
	};

}
