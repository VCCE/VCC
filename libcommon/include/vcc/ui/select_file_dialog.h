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
#include "vcc/ui/file_filter.h"
#include "vcc/detail/exports.h"
#include <Windows.h>
#include <filesystem>
#include <string>


namespace vcc::ui
{

	/// @brief A file selection dialog.
	///
	/// @todo this needs a better description.
	class select_file_dialog
	{
	public:

		/// @brief Type alias for a variable length string.
		using string_type = std::string;
		/// @brief Type alias for file paths.
		using path_type = std::filesystem::path;
		/// @brief Type alias for a file filter.
		using file_filter_type = ::vcc::ui::file_filter;
		/// @brief Type alias for a vector of file filters.
		using filter_vector_type = std::vector<file_filter_type>;


	public:

		/// @brief Construct the file selection dialog.
		LIBCOMMON_EXPORT select_file_dialog() = default;

		/// @brief Sets the title of the file selection dialog.
		/// 
		/// @param title A string containing the dialog title.
		/// 
		/// @return A reference to the file selection dialog.
		LIBCOMMON_EXPORT select_file_dialog& set_title(string_type title);

		/// @brief Sets the initial directory where to select files from.
		/// 
		/// @param initial_directory A string containing the initial directory.
		/// 
		/// @return A reference to the file selection dialog.
		LIBCOMMON_EXPORT select_file_dialog& set_initial_directory(const path_type& initial_directory);

		/// @brief Sets the file filter.
		/// 
		/// Sets the filters for the files so show in the file selection dialog.
		/// 
		/// @param filters A sequence of file filters
		/// 
		/// @return A reference to the file selection dialog.
		LIBCOMMON_EXPORT select_file_dialog& set_selection_filter(filter_vector_type filters);

		/// @brief The default extension.
		/// 
		/// do_modal_load_dialog and do_modal_save_dialog append this extension to the
		/// file name if the user fails to type an extension. 
		/// 
		/// @param extension The default extension.
		/// 
		/// @return A reference to the file selection dialog.
		LIBCOMMON_EXPORT select_file_dialog& set_default_extension(string_type extension);

		/// @brief Reset all flags to default values.
		/// 
		/// @return A reference to the file selection dialog.
		LIBCOMMON_EXPORT select_file_dialog& reset_flags();
		/// @brief Append flags.
		/// 
		/// For more information see the `Flags` section at
		/// https://learn.microsoft.com/en-us/windows/win32/api/commdlg/ns-commdlg-openfilenamea
		/// 
		/// @param flags One or more bit flags.
		/// 
		/// @return A reference to the file selection dialog.
		LIBCOMMON_EXPORT select_file_dialog& append_flags(unsigned int flags);

		/// @brief Retrieve the path selected by the user.
		/// 
		/// Retrieve the last path selected by the user. The selected path is only
		/// saved if the user selects "OK". IF the user cancels the file dialog no
		/// path is saved.
		/// 
		/// @return The path selected by the user.
		[[nodiscard]] LIBCOMMON_EXPORT path_type selected_path() const;

		/// @brief Creates and shows a _Save_ dialog
		/// 
		/// Creates a _Save_ dialog box that lets the user specify the drive, directory,
		/// and name of a file to save.
		/// 
		/// @param dialog_owner An optional handle to the parent window the file
		/// selection dialog.
		/// 
		/// @return `true` if the user clicked `OK`; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool do_modal_save_dialog(HWND dialog_owner = nullptr);

		/// @brief Creates and shows an _Open_ dialog.
		/// 
		/// Creates an _Open_ dialog box that lets the user specify the drive, directory,
		/// and the name of a file or set of files to be opened.
		/// 
		/// @param dialog_owner An optional handle to the parent window the file
		/// selection dialog.
		/// 
		/// @return `true` if the user clicked `OK`; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool do_modal_load_dialog(HWND dialog_owner = nullptr);


	private:

		/// @brief Creates and shows an _Open_ or _Save_ dialog.
		/// 
		/// Creates an _Open_ or _Save_ dialog box that lets the user specify the drive, directory,
		/// and the name of a file or set of files to be opened or saved.
		/// 
		/// @param use_save_dialog Flag indicating what type of dialog to create. If
		/// `true` the function will create and show a _Save_ dialog, otherwise it shows
		/// an _Open_ dialog.
		/// @param dialog_owner An optional handle to the parent window the file
		/// selection dialog.
		/// 
		/// @return `true` if the user clicked `OK`; `false` otherwise.
		[[nodiscard]] LIBCOMMON_EXPORT bool do_modal(
			bool use_save_dialog = false,
			HWND dialog_owner = nullptr);


	private:

		/// @brief The dialog title.
		string_type title_;
		/// @brief The initial directory to browse when selecting a file. Note this is
		/// stored as a string rather than a path to play nicely with the windows API.
		/// This should change when moving to all Unicode for localization.
		string_type initial_directory_;
		/// @brief A collection of file filters.
		filter_vector_type filters_;
		/// @brief The default exception.
		string_type default_extension_;
		/// @brief The open/save dialog flags.
		unsigned int flags_ = OFN_HIDEREADONLY;
		/// @brief The selected file returned by the open and save dialogs.
		string_type selected_file_;
	};

}
