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
#include <vcc/utils/filesystem.h>
#include <vcc/utils/winapi.h>


namespace vcc::utils
{


	LIBCOMMON_EXPORT std::string get_module_path(HMODULE module_handle)
	{
		std::string text(MAX_PATH, 0);

		for (;;)
		{
			DWORD ret = GetModuleFileName(module_handle, &text[0], text.size());
			if (ret == 0)
			{
				// An error occurred; return an empty string
				return {};
			}

			if (ret < text.size())
			{
				text.resize(ret);

				return text;
			}

			// Buffer was too small, double its size and try again
			text.resize(text.size() * 2);
		}
	}

	LIBCOMMON_EXPORT std::string find_pak_module_path(std::string path)
	{
		if (path.empty())
		{
			return path;
		}

		auto file_handle(CreateFile(path.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
		if (file_handle == INVALID_HANDLE_VALUE)
		{
			const auto application_path = get_directory_from_path(::vcc::utils::get_module_path());
			const auto alternate_path = application_path + path;
			file_handle = CreateFile(alternate_path.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file_handle == INVALID_HANDLE_VALUE)
			{
				path = alternate_path;
			}
		}

		CloseHandle(file_handle);

		return path;
	}

	LIBCOMMON_EXPORT std::string get_directory_from_path(std::string path)
	{
		const auto last_separator(path.find_last_of('\\'));
		if (last_separator != std::string::npos)
		{
			path.resize(last_separator + 1);
		}

		return path;
	}		

	LIBCOMMON_EXPORT std::string strip_application_path(std::string path)
	{
		const auto module_path = get_directory_from_path(vcc::utils::get_module_path(nullptr));
		auto temp_path(get_directory_from_path(path));
		if (module_path == temp_path)	// If they match remove the Path
		{
			path = get_filename(path);
		}

		return path;
	}

	LIBCOMMON_EXPORT std::string get_filename(std::string path)
	{
		const auto last_seperator = path.find_last_of('\\');
		if (last_seperator == std::string::npos)
		{
			return path;
		}

		path.erase(0, last_seperator + 1);

		return path;
	}

}
