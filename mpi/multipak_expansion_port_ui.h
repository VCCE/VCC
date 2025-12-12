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
#include "multipak_cartridge_driver.h"
#include "vcc/bus/expansion_port_ui.h"


namespace vcc::cartridges::multipak
{

	/// @inheritdoc
	class multipak_expansion_port_ui : public ::vcc::bus::expansion_port_ui
	{
	public:

		/// @brief Construct the UI service
		/// 
		/// @param host_ui A pointer to the cartridge hosts UI service.
		explicit multipak_expansion_port_ui(std::shared_ptr<::vcc::bus::expansion_port_ui> host_ui)
			: host_ui_(move(host_ui))
		{
			if (host_ui_ == nullptr)
			{
				throw std::invalid_argument("Cannot construct Multi-Pak UI Service Shim. UI is null.");
			}
		}

		/// @inheritdoc
		HWND app_window() const noexcept override
		{
			return host_ui_->app_window();
		}


	private:

		/// @brief The cartridge hosts UI service.
		std::shared_ptr<::vcc::bus::expansion_port_ui> host_ui_;
	};

}
