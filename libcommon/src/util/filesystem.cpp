#include <vcc/util/fileutil.h>
#include <vcc/util/filesystem.h>

//TODO filesystem.cpp is depreciated
//TODO move this it is used only in mpi/multipak_cartridge.cpp:

namespace VCC::Util
{
	std::string find_pak_module_path(std::string path)
	{
		if (path.empty())
		{
			return path;
		}

		auto file_handle(CreateFile(path.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
		if (file_handle == INVALID_HANDLE_VALUE)
		{
			const auto application_path = ::VCC::Util::GetDirectoryPart(::VCC::Util::get_module_path(NULL));
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
}
