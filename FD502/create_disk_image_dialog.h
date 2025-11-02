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
#include "floppy_disk_layout.h"
#include "vcc/ui/dialog_window.h"
#include <map>


namespace vcc::cartridges::fd502
{

	/// @brief Dialog window for creating new disk images.
	/// 
	/// This class provides the user interface for creating a new disk image. It supports
	/// creating disks in a variety of formats including DSK and DMK. In addition to the
	/// variety of formats the user is also able to select the number of tracks and whether
	/// the disk is single or double sided.
	class create_disk_image_dialog : public ::vcc::ui::dialog_window
	{
	public:

		/// @brief Construct the Create Disk Image dialog.
		/// 
		/// @param module_instance The instance of the library module containing the dialog
		/// resources.
		/// @param image_filename The filename of the disk image to create.
		create_disk_image_dialog(
			HINSTANCE module_instance,
			path_type image_filename);


	protected:

		/// @inheritdoc
		bool on_init_dialog() override;

		/// @inheritdoc
		INT_PTR on_command(WPARAM wParam, LPARAM lParam) override;


	private:

		/// @brief Type alias for disk image format identifiers.
		using disk_image_format_type = ::vcc::cartridges::fd502::detail::disk_image_format_id;

		/// @brief Defines default values for disk creation settings.
		struct defaults
		{
			/// @brief Defines the default disk image layout used to format the disk image.
			static const auto image_layout = disk_image_format_type::JVC;
			/// @brief Defines the default setting for creating double sided disk images.
			static const auto double_sided = false;
			/// @brief Defines the default number of tracks.
			static const auto track_count = 35;
		};

		/// @brief Table used for converting UI control identifiers to disk image format
		/// identifiers.
		static const std::map<UINT, disk_image_format_type> disk_type_id_to_enum_map_;
		/// @brief Table used for converting disk image format identifiers to UI control 
		/// identifiers.
		static const std::map<disk_image_format_type, UINT> disk_type_enum_map_to_id_;
		/// @brief Table used for converting UI control identifiers to track count values.
		static const std::map<UINT, size_type> track_count_id_to_value_;
		/// @brief Table used for converting track count values to UI control identifiers.
		static const std::map<size_type, UINT> track_count_value_to_id_;

		/// @brief The filename of the disk image to create.
		const path_type image_filename_;
		/// @brief The disk image format used to create the disk image.
		disk_image_format_type disk_image_layout_ = defaults::image_layout;
		/// @brief Flag indicating if the disk image should be created as a double sided
		/// disk.
		bool double_sided_ = defaults::double_sided;
		/// @brief The number of tracks the disk image will contain.
		size_type track_count_ = defaults::track_count;
	};

}
