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


namespace vcc::cartridges::fd502::detail
{

	/// @brief Represents the register set of the WD179x floppy disk controller.
	struct fdc_register_file
	{
	public:

		/// @brief Type alias for register values.
		using register_value_type = unsigned char;


	public:

		/// @brief This 8-bit register holds the command presently being executed. This
		/// register should not be loaded when the device is busy unless the new command is a
		/// force interrupt. The command register can be loaded from the DAL, but not read
		/// onto the DAL.
		register_value_type command = 0;
		/// @brief This 8-bit register holds device Status information. The meaning of the
		/// Status bits is a function of the type of command previously executed. This register
		/// can be read onto the DAL, but not loaded from the DAL.
		register_value_type status = 0;
		/// @brief This 8-bit register holds the track number of the current Read/Write head
		/// position. It is incremented by one every time the head is stepped in (towards
		/// track 76) and decremented by one when the head is stepped out (towards track 00).
		/// The contents of the register are compared with the recorded track number in the
		/// ID field during disk Read, Write and Verify operations. The Track Register can be
		/// loaded from or transferred to the DAL. This Register should not be loaded when
		/// the device is busy.
		register_value_type track = 0;
		/// @brief This 8-bit register holds the address of the desired sector position. The
		/// contents of the register are compared with the recorded sector number in the ID
		/// field during disk Read or Write operations. The Sector Register contents can be
		/// loaded from or transferred to the DAL. This register should not be loaded when
		/// the device is busy.
		register_value_type sector = 0;
		/// @brief This 8-bit register is used as a holding register during Disk Read and
		/// Write operations. In Disk Read operations the assembled data byte is transferred
		/// in parallel to the Data Register from the Data Shift Register. In Disk Write
		/// operations information is transferred in parallel from the Data Register to the
		/// Data Shift Register.
		/// 
		/// When executing the Seek command the Data Register holds the address of the desired
		/// Track position. This register is loaded from the DAL and gated onto the DAL under
		/// processor control.
		register_value_type data = 0;
	};

}
