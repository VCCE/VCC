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
#include "cartridge_slot.h"
#include "configuration_dialog.h"
#include "multipak_configuration.h"
#include <vcc/bus/cartridge.h>
#include <vcc/utils/critical_section.h>
#include "../CartridgeMenu.h"
#include <array>
extern HINSTANCE gModuleInstance;

std::string mpi_slot_label(std::size_t slot);
std::string mpi_slot_description(std::size_t slot);

bool mpi_empty(std::size_t slot);

void mpi_eject_cartridge(std::size_t slot);
::vcc::utils::cartridge_loader_status mpi_mount_cartridge(std::size_t slot, const std::string& filename);

void mpi_switch_to_slot(std::size_t slot);
std::size_t mpi_selected_switch_slot();
std::size_t mpi_selected_scs_slot();
void mpi_append_menu_item(std::size_t slot, CartMenuItem item);
void mpi_assert_cartridge_line(std::size_t slot, bool line_state);

void mpi_build_menu();
