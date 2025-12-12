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
#include <Windows.h>


// DO NOT DOCUMENT UNTIL THIS GARBAGE IS REFACTORED
#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace vcc::devices::rtc
{

	class LIBCOMMON_EXPORT ds1315
	{
	public:

		void set_read_only(bool value);

		[[nodiscard]] unsigned char read_port(unsigned short port_id);


	private:

		void set_time();


	private:

		SYSTEMTIME now;
		unsigned __int64 InBuffer = 0;
		unsigned __int64 OutBuffer = 0;
		unsigned char BitCounter = 0;
		unsigned char TempHour = 0;
		unsigned char AmPmBit = 0;
		unsigned __int64 CurrentBit = 0;
		unsigned char FormatBit = 0; //1 = 12Hour Mode
		unsigned char CookieRecived = 0;
		unsigned char WriteEnabled = 0;
	};

}

#endif
