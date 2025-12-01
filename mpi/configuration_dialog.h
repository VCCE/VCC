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
#include "multipak_configuration.h"
#include "multipak_cartridge_driver.h"
#include "vcc/utils/cartridge_loader.h"
#include <Windows.h>
#include <functional>


namespace vcc::cartridges::multipak
{

	/// @brief The Multi-Pak Slot Manager
	///
	/// @todo rename to multipak_slot_manager_dialog
	class configuration_dialog
	{
	public:

		/// @brief Type alias for slot indexes and identifiers.
		using slot_id_type = ::vcc::cartridges::multipak::multipak_cartridge_driver::slot_id_type;
		/// @brief Type alias for the function called to insert a cartridge in the Multi-Pak.
		using insert_function_type = std::function<void(slot_id_type)>;
		/// @brief Type alias for the function called to eject a cartridge from the Multi-Pak.
		using eject_function_type = std::function<void(slot_id_type)>;


	public:

		/// @brief Construct the Configuration Dialog
		/// 
		/// @param module_handle A handle to the instance of the module containing the
		/// resources for the configuration dialog.
		/// @param configuration A pointer to the configuration for this instance.
		/// @param mpi A pointer to the Multi-Pak driver the configuration is for.
		/// @param insert_callback A function that performs a cartridge insert on the
		/// Multi-Pak.
		/// @param eject_callback A function that performs a cartridge eject on the
		/// Multi-Pak.
		/// 
		/// @throws std::invalid_argument if `module_instance` is null.
		/// @throws std::invalid_argument if `configuration` is null.
		/// @throws std::invalid_argument if `mpi` is null.
		/// @throws std::invalid_argument if `insert_callback` is empty.
		/// @throws std::invalid_argument if `eject_callback` is empty.
		configuration_dialog(
			HINSTANCE module_handle,
			std::shared_ptr<multipak_configuration> configuration,
			std::shared_ptr<multipak_cartridge_driver> mpi,
			insert_function_type insert_callback,
			eject_function_type eject_callback);

		/// @brief Opens the dialog.
		///
		/// Opens the configuration dialog. If the dialog is already open, it is brought to the
		/// top of the Z-order and presented to the user.
		void open();

		/// @brief Closes the dialog.
		///
		/// Closes the dialog if it is open.
		void close();

		/// @brief Updates the visual elements that show which slot is selected as the
		/// active/startup slot.
		void update_selected_slot();

		/// @brief Updates the details of a slot.
		/// 
		/// Updates the details, including the details of the cartridge currently loaded, for
		/// a specific slot.
		/// 
		/// @param slot The slot to update.
		void update_slot_details(slot_id_type slot);


	private:

		/// @brief Sets the active/startup slot of the Multi-Pak.
		/// 
		/// @param slot The new active/startup slot.
		void set_selected_slot(slot_id_type slot);

		/// @brief Eject or insert a new cartridge.
		/// 
		/// @param slot The slot to eject or add the cartridge to.
		void eject_or_select_new_cartridge(slot_id_type slot);

		/// @brief Callback procedure that processes dialog messages.
		/// 
		/// @param hDlg A handle to the dialog window the message is for.
		/// @param message The message.
		/// @param wParam The word parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// @param lParam The long parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// 
		/// @return The return value depends on the message being processed.
		static INT_PTR CALLBACK callback_procedure(
			HWND hDlg,
			UINT message,
			WPARAM wParam,
			LPARAM lParam);

		/// @brief Process a dialog message.
		/// 
		/// @param hDlg A handle to the dialog window the message is for.
		/// @param message The message.
		/// @param wParam The word parameter for the message. The contents of this parameter
		/// depend on the message received.
		/// 
		/// @return The return value depends on the message being processed.
		INT_PTR process_message(
			HWND hDlg,
			UINT message,
			WPARAM wParam,
			LPARAM lParam);

	private:

		/// @brief The handle to the module instance containing the cartridges resources.
		const HINSTANCE module_handle_;
		/// @brief The Multi-Pak configuration.
		const std::shared_ptr<multipak_configuration> configuration_;
		/// @brief A pointer to the Multi-Pak driver the configuration is for.
		const std::shared_ptr<multipak_cartridge_driver> mpi_;
		/// @brief A function that performs a cartridge insert on the
		const insert_function_type insert_callback_;
		/// @brief A function that performs a cartridge eject on the
		const eject_function_type eject_callback_;
		/// @brief The handle to the currently opened dialog, or null if no dialog is open.
		HWND dialog_handle_ = nullptr;
		/// @brief The parent window of the currently opened dialog, or null if no dialog is open.
		HWND parent_handle_ = nullptr;	//	TODO-CHET: This is only used for one thing. Delete it!
	};

}