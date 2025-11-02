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
#include "fd502_configuration.h"
#include "fd502_cartridge_driver.h"
#include "disk_basic_rom_image_id.h"
#include "vcc/ui/dialog_window.h"
#include "vcc/utils/cartridge_loader.h"
#include <functional>
#include <array>


namespace vcc::cartridges::fd502
{

	/// @brief Modaless dialog window providing a settings UI for the FD502.
	///
	/// This class provides the user interface for the FD502 and WD179x settings. It allows
	/// the selection of the Disk BASIC ROM file, whether to serialize disk mount settings,
	/// and control of integrated devices such as the Disto Real Time Clock and the Becker
	/// Port.
	class configuration_dialog : public ::vcc::ui::dialog_window
	{
	public:

		/// @brief Type alias for identifiers of supported Disk BASIC ROM images.
		using rom_image_id_type = ::vcc::cartridges::fd502::detail::disk_basic_rom_image_id;
		/// @brief TYpe alias for the function type used to notify of settings changes.
		using settings_changed_function_type = std::function<void()>;


	public:

		/// @brief Construct the Configuration Dialog
		/// 
		/// @param module_handle A handle to the instance of the module containing the
		/// resources for the configuration dialog.
		/// @param configuration A pointer to the configuration for this instance.
		/// @param driver A pointer to the FD502 the configuration is for.
		/// @param settings_changed A function that will be invoked when the user accepts
		/// the settings by clicking OK.
		/// 
		/// @throws std::invalid_argument if `module_instance` is null.
		/// @throws std::invalid_argument if `configuration` is null.
		/// @throws std::invalid_argument if `driver` is null.
		/// @throws std::invalid_argument if `settings_changed` is empty.
		configuration_dialog(
			HINSTANCE module_handle,
			std::shared_ptr<fd502_configuration> configuration,
			std::shared_ptr<fd502_cartridge_driver> driver,
			settings_changed_function_type settings_changed);


	protected:

		/// @inheritdoc
		bool on_init_dialog() override;

		/// @inheritdoc
		INT_PTR on_command(WPARAM wParam, LPARAM lParam) override;

		/// @brief Saves the changed settings, dispatches settings change notifications and
		/// closes the window.
		void on_ok() override;

		/// @brief Opens a file selection window allowing the user to select the Disk BASIC
		/// ROM to use.
		void on_browse_for_rom_image();


	private:

		/// @brief The Multi-Pak configuration.
		const std::shared_ptr<fd502_configuration> configuration_;
		/// @brief A pointer to the Multi-Pak driver the configuration is for.
		const std::shared_ptr<fd502_cartridge_driver> driver_;
		/// @brief The function to call when the settings are accepted by the user and saved.
		const settings_changed_function_type settings_changed_;
		/// @brief The identifier of the Disk BASIC ROM to use.
		rom_image_id_type selected_rom_id_ = rom_image_id_type::microsoft;
	};

}
