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
#include "becker_cartridge.h"
#include "configuration_dialog.h"
#include "resource.h"
#include <vcc/devices/serial/beckerport.h>
#include <vcc/bus/cartridge_capi.h>
#include <vcc/utils/winapi.h>
#include <vcc/utils/persistent_value_store.h>
#include "../CartridgeMenu.h"
#include <Windows.h>

// Contains Becker cart exports
HINSTANCE gModuleInstance;

namespace
{
	using becker_device_type = ::vcc::devices::serial::Becker;

	struct menu_identifiers
	{
		static const UINT open_configuration = 16;
	};

	const std::string configuration_section_id_ = "DW Becker";

	void* gHostKey = nullptr;
	const cartridge_capi_context* context_ = nullptr;
	std::string configuration_path_;
	configuration_dialog configuration_dialog_;
	becker_device_type gBecker;
}

static void build_menu();


// Becker dll main
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		gModuleInstance = hinstDLL;
	}

	return true;
}

extern "C"
{

	__declspec(dllexport) const char* PakGetName()
	{
		static const auto name(::vcc::utils::load_string(gModuleInstance, IDS_MODULE_NAME));
		return name.c_str();
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static const auto catalog_id(::vcc::utils::load_string(gModuleInstance, IDS_CATNUMBER));
		return catalog_id.c_str();
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static const auto description(::vcc::utils::load_string(gModuleInstance, IDS_DESCRIPTION));
		return description.c_str();
	}


	__declspec(dllexport) void PakInitialize(
		void* const host_key,
		const char* const configuration_path,
		const cartridge_capi_context* const context)
	{
		gHostKey = host_key;
		configuration_path_ = configuration_path;
		context_ = context;

		::vcc::utils::persistent_value_store settings(configuration_path_);

		gBecker.sethost(
			settings.read(configuration_section_id_, "DWServerAddr", "127.0.0.1").c_str(),
			settings.read(configuration_section_id_, "DWServerPort", "65504").c_str());
		gBecker.enable(true);

		build_menu();
	}


	__declspec(dllexport) void PakTerminate()
	{
		gBecker.enable(false);
		configuration_dialog_.close();
	}


	__declspec(dllexport) void PakWritePort(unsigned char port_id, unsigned char value)
	{
		if (port_id == 0x42)
		{
			gBecker.write(value, port_id);
		}
	}

	__declspec(dllexport) unsigned char PakReadPort(unsigned char port_id)
	{
		switch (port_id)
		{
		case 0x41:	// read status
			return gBecker.read(port_id) != 0 ? 2 : 0;

		case 0x42:	// read data
			return gBecker.read(port_id);
		}

		return 0;
	}

	__declspec(dllexport) void PakGetStatus(char* text_buffer, [[maybe_unused]] size_t buffer_size)
	{
		gBecker.status(text_buffer);  // text buffer size??
	}


	__declspec(dllexport) void PakMenuItemClicked(unsigned char menu_item_id)
	{
		if (menu_item_id == menu_identifiers::open_configuration)
		{
			configuration_dialog_.open();
		}
	}

}


static void build_menu()
{
	context_->add_menu_item(gHostKey, "", MID_BEGIN, MIT_Head);
	context_->add_menu_item(gHostKey, "", MID_ENTRY, MIT_Seperator);
	context_->add_menu_item(gHostKey, "DriveWire Server..", ControlId(menu_identifiers::open_configuration), MIT_StandAlone);
	context_->add_menu_item(gHostKey, "", MID_FINISH, MIT_Head);
}


std::string becker_server_address()
{
	return gBecker.server_address();
}

std::string becker_server_port()
{
	return gBecker.server_port();
}

// Save becker config
void becker_configure_server(std::string server_address, std::string server_port)
{
	gBecker.sethost(server_address.c_str(), server_port.c_str());

	::vcc::utils::persistent_value_store settings(configuration_path_);

	settings.write(configuration_section_id_, "DWServerAddr", server_address);
	settings.write(configuration_section_id_, "DWServerPort", server_port);
}
