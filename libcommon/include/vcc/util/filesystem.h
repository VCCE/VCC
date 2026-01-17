#pragma once
#include <vcc/util/exports.h>
#include <string>
#include <Windows.h>

//TODO: depreciate this - it is used only in mpi/multipak_cartridge.cpp:
namespace VCC::Util
{
	LIBCOMMON_EXPORT std::string find_pak_module_path(std::string path);
}
