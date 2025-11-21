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


namespace vcc::bus
{

	/// @brief Interface to the user interface services of the system managing the
	/// expansion port.
	///
	/// @todo rename to expansion_port_ux
	/// 
	/// The Expansion Port User Experience abstract class defines the user experience
	/// services and properties provided by the system managing the expansion port.
	class LIBCOMMON_EXPORT expansion_port_ui
	{
	public:

		virtual ~expansion_port_ui() = default;
	};

}
