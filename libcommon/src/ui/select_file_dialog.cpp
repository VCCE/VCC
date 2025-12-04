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
#include "vcc/ui/select_file_dialog.h"
#include <numeric>
#include <array>


namespace vcc::ui
{

	select_file_dialog& select_file_dialog::set_title(string_type title)
	{
		title_ = move(title);

		return *this;
	}

	select_file_dialog& select_file_dialog::set_initial_directory(const path_type& initial_directory)
	{
		initial_directory_ = initial_directory.string();

		return *this;
	}

	select_file_dialog& select_file_dialog::set_selection_filter(filter_vector_type filters)
	{
		filters_ = move(filters);

		return *this;
	}

	select_file_dialog& select_file_dialog::set_default_extension(string_type extension)
	{
		default_extension_ = move(extension);

		return *this;
	}

	select_file_dialog& select_file_dialog::reset_flags()
	{
		flags_ = OFN_HIDEREADONLY;

		return *this;
	}

	select_file_dialog& select_file_dialog::append_flags(unsigned int flags)
	{
		flags_ |= flags;

		return *this;
	}

	select_file_dialog::path_type select_file_dialog::selected_path() const
	{
		return selected_file_;
	}

	bool select_file_dialog::do_modal_save_dialog(HWND dialog_owner)
	{
		return do_modal(true, dialog_owner);
	}

	bool select_file_dialog::do_modal_load_dialog(HWND dialog_owner)
	{
		return do_modal(false, dialog_owner);
	}

	bool select_file_dialog::do_modal(bool use_save_dialog, HWND dialog_owner)
	{
		string_type filters_string;

		for (const auto& [description, filters] : filters_)
		{
			string_type concatenated_filters = std::accumulate(
				std::next(filters.begin()),
				filters.end(),
				filters[0],
				[](const std::string& a, const std::string& b)
				{
					return a + ";" + b;
				});

			filters_string += description + " (" + concatenated_filters + ")";
			filters_string.push_back(0);
			filters_string += concatenated_filters;
			filters_string.push_back(0);
		}
		filters_string.push_back(0);


		OPENFILENAME dialog_params = {};
		std::array<string_type::value_type, 2048> path_buffer = {};

		dialog_params.lStructSize = sizeof(dialog_params);
		// TODO-CHET: Should we use the active window or just leave it null?
		dialog_params.hwndOwner = dialog_owner ? dialog_owner : GetActiveWindow();
		dialog_params.hInstance = GetModuleHandle(nullptr);
		dialog_params.lpstrFilter = filters_string.c_str();
		dialog_params.lpstrFile = path_buffer.data();
		dialog_params.nMaxFile = path_buffer.size();
		dialog_params.lpstrInitialDir = initial_directory_.c_str();
		dialog_params.lpstrTitle = title_.c_str();
		dialog_params.Flags = flags_;
		dialog_params.lpstrDefExt = default_extension_.c_str();

		const auto result(
			use_save_dialog
			? GetSaveFileName(&dialog_params)
			: GetOpenFileName(&dialog_params));
		if (result == FALSE)
		{
			return false;
		}

		selected_file_ = dialog_params.lpstrFile;

		return true;
	}

}
