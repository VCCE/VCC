
#ifndef __CC3VHD_H__
#define __CC3VHD_H__
/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

// cc3vhd.h
#include "vcc/bus/cartridge_context.h"

void UnmountHD(int);
int MountHD(const char*, int);
unsigned char IdeRead(unsigned char);
void IdeWrite (::vcc::bus::cartridge_context& context, unsigned char, unsigned char);
void DiskStatus(char* text_buffer, size_t buffer_size);
void VhdReset(::vcc::bus::cartridge_context& context);

constexpr auto DRIVESIZE = 512u; // Mb
constexpr auto MAX_SECTOR = DRIVESIZE * 1024 * 1024;
constexpr auto SECTORSIZE = 256u;

// FIXME: These need should probably be turned into a scoped enum.
constexpr auto HD_OK	= 0;
constexpr auto HD_PWRUP	= -1;
constexpr auto HD_INVLD	= -2;
constexpr auto HD_NODSK	= 4;
constexpr auto HD_WP	= 5;

// FIXME: These need should probably be turned into a scoped enum.
constexpr auto SECTOR_READ	= 0u;
constexpr auto SECTOR_WRITE	= 1u;
constexpr auto DISK_FLUSH	= 2u;

#endif
