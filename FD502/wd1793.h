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
#include "defines.h"
#include "vcc/bus/cartridge_context.h"
#include <Windows.h>
#include <string>


unsigned char disk_io_read(::vcc::bus::cartridge_context& context, unsigned char port);
void disk_io_write(::vcc::bus::cartridge_context& context, unsigned char data,unsigned char port);	
int mount_disk_image(const char *,unsigned char );
void unmount_disk_image(unsigned char drive);
void DiskStatus(char* text_buffer, size_t buffer_size);
void PingFdc(::vcc::bus::cartridge_context& context);
unsigned char SetTurboDisk( unsigned char);
//unsigned char UseKeyboardLeds(unsigned char);
DWORD GetDriverVersion ();
unsigned short InitController ();
std::string get_mounted_disk_filename(::std::size_t drive_index);

// FIXME: Needs a name and should be scoped
enum
{
	JVC,
	VDK,
	DMK,
	OS9,
	RAW
};
