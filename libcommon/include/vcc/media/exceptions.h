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
#include "vcc/detail/exports.h"
#include <stdexcept>


namespace vcc::media
{
	
	/// @brief Exception raised when IO error the system cannot or should not recover from.
	class fatal_io_error : public std::runtime_error
	{
	public:

		using std::runtime_error::runtime_error;
	};

	/// @brief Exception raised when an operation on a buffer has been requested but the
	/// size of the buffer is incorrect or not sufficient for the operation.
	class buffer_size_error : public std::runtime_error
	{
	public:

		using std::runtime_error::runtime_error;
	};

	/// @brief Exception raised when an attempt to write to media that is wrote protected
	/// is attempted.
	class write_protect_error : public std::runtime_error
	{
	public:

		using std::runtime_error::runtime_error;
	};

	/// @brief Exception raised when an invalid sector record is encountered.
	class sector_record_error : public std::runtime_error
	{
	public:

		using std::runtime_error::runtime_error;
	};

}
