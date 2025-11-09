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
#include <vcc/core/detail/exports.h>
#include <Windows.h>


namespace vcc::devices::serial
{
	constexpr int BUFFER_SIZE = 512;

	class LIBCOMMON_EXPORT Becker
	{
	public:
		Becker();
		~Becker();

		const char* server_address() const;
		const char* server_port() const;

		void enable(bool);                        // enable or disable
		void sethost(const char *, const char *); // server ip address, port
		unsigned char read(unsigned short);       // coco port
		void write(unsigned char,unsigned short); // value, coco port
		void status(char *);                      // becker status for status line

	private:
		// Thread entry point
		static unsigned __stdcall threadEntry(void *);
        unsigned __stdcall runThread(HANDLE hEvent);

		// helpers
		void dw_open();
		void dw_close();
		//SOCKET dw_open(const char *, const char *);
		unsigned char dw_status();
		unsigned char dw_read();
		int dw_write(char data);

		// state
		SOCKET socket_ = 0;
		bool enabled_ = false;
		bool retry_ = false;
		HANDLE threadHandle_ = nullptr;

		// circular buffer for socket io
		char buffer_[BUFFER_SIZE];
		int read_pos_ = 0;
		int write_pos_ = 0;

		// hostname and port
		static char address_[512];
		static char port_[16];
		static char cur_address_[512];
		static char cur_port_[16];

		// statistics
		int bytes_written_ = 0;
		int bytes_read_ = 0;
		DWORD last_stats_ = 0;
		float read_speed_ = 0;
		float write_speed_ = 0;
	};
}
