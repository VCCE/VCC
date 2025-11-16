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
#include "vcc/bus/cartridge.h"
#include "vcc/bus/cartridge_context.h"
#include "vcc/devices/rom/rom_image.h"
#include "resource.h"
#include <stdio.h>
#include <conio.h>
#include <windowsx.h>
#include <Windows.h>
#include <memory>


class rs232pak_cartridge : public ::vcc::bus::cartridge
{
public:

	using context_type = ::vcc::bus::cartridge_context;
	using rom_image_type = ::vcc::devices::rom::rom_image;

	rs232pak_cartridge(std::unique_ptr<context_type> context, HINSTANCE module_instance);

	/// @inheritdoc
	name_type name() const override;

	/// @inheritdoc
	catalog_id_type catalog_id() const override;

	/// @inheritdoc
	description_type description() const override;


	void start() override;
	void stop() override;
	void reset() override;

	unsigned char read_memory_byte(size_type memory_address) override;

	void write_port(unsigned char port_id, unsigned char value) override;
	[[nodiscard]] unsigned char read_port(unsigned char port_id) override;

	void status(char* text_buffer, size_type buffer_size) override;
	void menu_item_clicked(unsigned char menu_item_id) override;
	void update(float delta) override;


private:

	//void LoadConfig();
	void BuildCartridgeMenu();

private:

	const std::unique_ptr<context_type> context_;
	const HINSTANCE module_instance_;
};




// Text mode EOF character
constexpr auto EOFCHR = 0x1Bu;

// Base Ports
constexpr auto BASE_PORT_RS232 = 0x68u;
constexpr auto BASE_PORT_MODEM = 0x6Cu;

// Communications type and mode enumerations
enum com_type {
    COM_CONSOLE,
    COM_FILE,
    COM_TCPIP,
    COM_WINCOM
};
enum com_mode {
    COM_MODE_DUPLEX,
    COM_MODE_READ,
    COM_MODE_WRITE,
};

// Handles for I/O threads
extern HANDLE hInputThread;
extern HANDLE hOutputThread;
extern HANDLE hStopInput;
extern HANDLE hStopOutput;

extern int  AciaBasePort;             // Base port for sc6651 (0x68 or 0x6C)
extern int  AciaComType;              // Console,file,tcpip,wincom
extern int  AciaComMode;              // Duplex,read,write
extern int  AciaTextMode;             // CR and EOF translations 0=none 1=text
extern int  AciaLineInput;            // Console line mode 0=Normal 1=Linemode
extern char AciaTcpPort[32];          // TCP port 1024-65536
extern char AciaComPort[32];          // Windows Serial port eg COM20
extern char AciaTcpHost[MAX_PATH];    // Tcpip hostname
extern char AciaFileRdPath[MAX_PATH]; // Path for file reads
extern char AciaFileWrPath[MAX_PATH]; // Path for file writes

extern void* const& gHostKey;

// Device
extern void sc6551_init();
extern void sc6551_close();
void sc6551_heartbeat(::vcc::bus::cartridge_context& context);
extern unsigned char sc6551_read(unsigned char data);
extern void sc6551_write(unsigned char data, unsigned short port);

// Comunications hooks
extern int  com_open();
extern void com_close();
extern int  com_write(const char*,int);
extern int  com_read(char*,int);

// Console
extern int  console_open();
extern void console_close();
extern int  console_read(char*,int);
extern int  console_write(const char*,int);

// File
extern int  file_open();
extern void file_close();
extern int  file_read(char*,int);
extern int  file_write(const char*,int);

// Tcpip
extern int  tcpip_open();
extern void tcpip_close();
extern int  tcpip_read(char*,int);
extern int  tcpip_write(const char*,int);

// WinCom 
extern int  wincom_open();
extern void wincom_close();
extern int  wincom_read(char*,int);
extern int  wincom_write(const char*,int);

