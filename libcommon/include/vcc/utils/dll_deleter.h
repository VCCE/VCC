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
#include <vcc/utils/logger.h>
#include <Windows.h>


namespace vcc::utils
{

	/// @brief Custom `std::unique_ptr` "deleter" for managing the lifetime of shared libraries.
	struct LIBCOMMON_EXPORT dll_deleter
	{
		/// @brief Release a shared library.
		/// 
		/// @param instance The handle to the shared library to release.
		void operator()(HMODULE instance) const noexcept
		{
			if (instance != nullptr)
			{
				[[maybe_unused]] const auto result(FreeLibrary(instance));
				DLOG_C("pak:err FreeLibrary %d %d\n", instance, result);
			}
		};
	};

}
