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


// DO NOT DOCUMENT UNTIL THIS GARBAGE IS REFACTORED
#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace vcc::devices::rtc
{

	class LIBCOMMON_EXPORT oki_m6242b
	{
	public:

		void enable(bool state);
		bool enabled() const;

		void set_read_write_address(size_t address);

		void write_data(unsigned char value);
		[[nodiscard]] unsigned char read_data() const;


	private:

		bool enabled_ = false;
		unsigned char time_register_ = 0;
		unsigned char hour12_ = 0;
	};

}

#endif
