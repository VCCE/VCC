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
#include <vcc/core/fileutil.h>
#include <vcc/core/filesystem.h>
#include <vcc/core/winapi.h>

//TODO filesystem.cpp should be depreciated
//TODO replace get_directory_from_path and get_filename with fileutil functions
//TODO move find_pak_module_path to point of use

namespace vcc::core::utils
{
	// TODO: move this
	LIBCOMMON_EXPORT std::string find_pak_module_path(std::string path)
	{
		if (path.empty())
		{
			return path;
		}

		auto file_handle(CreateFile(path.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
		if (file_handle == INVALID_HANDLE_VALUE)
		{
			const auto application_path = get_directory_from_path(::VCC::Util::get_module_path(NULL));
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

	// TODO: replace with ::VCC::Util::GetDirectoryPart(const std::string& input)
	LIBCOMMON_EXPORT std::string get_directory_from_path(std::string path)
	{
		const auto last_separator(path.find_last_of('\\'));
		if (last_separator != std::string::npos)
		{
			path.resize(last_separator + 1);
		}

		return path;
	}		

	//TODO: replace with ::VCC::Util::GetFileNamePart(const std::string& input)
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
