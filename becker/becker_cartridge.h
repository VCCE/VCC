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
#include "becker_cartridge_driver.h"
#include "configuration_dialog.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/expansion_port_host.h"
#include "vcc/bus/expansion_port_ui.h"
#include "vcc/bus/expansion_port_bus.h"
#include <Windows.h>
#include <memory>


/// @brief Cartridge implementation of the Becker Port cartridge.
///
/// Cartridge implementation connecting the Becker Port device to the emulated system.
class becker_cartridge : public ::vcc::bus::cartridge
{
public:

	/// @brief Defines the type representing the expansion port bus.
	using expansion_port_bus_type = ::vcc::bus::expansion_port_bus;
	/// @brief Defines the type providing user interface services to the cartridge.
	using expansion_port_ui_type = ::vcc::bus::expansion_port_ui;
	/// @brief Defines the type providing system services to the cartridge.
	using expansion_port_host_type = ::vcc::bus::expansion_port_host;
	/// @brief Defines the primary driver type the cartridge controls.
	using driver_type = becker_cartridge_driver;
	/// @brief Defines the type used to hold a variable length string.
	using string_type = std::string;


public:

	/// @brief Construct the Becker Port cartridge.
	/// 
	/// @param host A pointer to the host interface.
	/// @param module_instance A handle to the instance of the module containing the
	/// Becker Port resources.
	becker_cartridge(
		std::shared_ptr<expansion_port_host_type> host,
		HINSTANCE module_instance);

	/// @inheritdoc
	name_type name() const override;

	/// @inheritdoc
	[[nodiscard]] driver_type& driver() override;

	/// @inheritdoc
	void start() override;

	/// @inheritdoc
	void stop() override;

	/// @inheritdoc
	menu_item_collection_type get_menu_items() const override;

	/// @inheritdoc
	void menu_item_clicked(unsigned char menu_item_id) override;

	/// @inheritdoc
	void status(char* text_buffer, size_t buffer_size) override;


protected:

	/// @brief Retrieve the address of the DriveWire server the Becker driver will connect to.
	/// 
	/// @return A string containing the server address.
	string_type server_address_setting() const;

	/// @brief Retrieve the port of the DriveWire server the Becker driver will connect to.
	/// 
	/// @return A string containing the server port.
	string_type server_port_setting() const;

	/// @brief Updates the address and port of the DriveWrite server to connect to.
	/// 
	/// @param server_address The address of the DriveWrite server to connect to.
	/// @param server_port The port of the DriveWire server to connect to.
	void update_connection_settings(
		const string_type& server_address,
		const string_type& server_port);


private:

	/// @brief Defines the identifiers representing the options available to the
	/// user in the emulator's cartridge menu.
	struct menu_item_ids
	{
		/// @brief Requests the Becker Port Cartridge open its settings menu.
		static const UINT open_settings = 16;
	};

	struct configuration
	{
		/// @brief Defines the section where all settings for the Becker Cartridge
		/// are stored.
		static const inline std::string section = "Cartridges.BeckerPort";
		/// @brief Defines the keys settings values are stored in.
		struct keys
		{
			/// @brief The key used to store the address of the DriveWire server to
			/// connect to.
			static const inline std::string server_address = "server.address";
			/// @brief The key used to store the port number of the DriveWire server to
			/// connect to.
			static const inline std::string server_port = "server.port";
		};

		/// @brief Defines default settings values.
		struct defaults
		{
			/// @brief The default DriveWire server address to connect to.
			static const inline std::string server_address = "127.0.0.1";
			/// @brief The default DriveWire server port to connect to.
			static const inline std::string server_port = "65504";
		};
	};


private:

	const std::shared_ptr<expansion_port_host_type> host_;
	const HINSTANCE module_instance_;
	configuration_dialog configuration_dialog_;
	driver_type driver_;
};
