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
//#define USE_LOGGING

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <Windows.h>
#include <ws2tcpip.h>
#include <process.h>
#include <string>

#include <vcc/common/logger.h>
#include <vcc/devices/becker/beckerport.h>

namespace vcc::devices::beckerport
{

	constexpr int TCP_RETRY_DELAY = 500;
	constexpr int STATS_PERIOD_MS = 100;

	Becker::Becker() {
    	last_stats_ = GetTickCount();  // Overflow after 49 days
	}

	Becker::~Becker() {
    	enable(false);
	}

	// Defaults for port and address
	char Becker::address_[512] = "localhost";
	char Becker::cur_address_[512] = "localhost";
	char Becker::port_[16] = "65504";
	char Becker::cur_port_[16] = "65504";

	// Set address and port
	void Becker::sethost(const char* addr, const char* port)
	{
		strcpy(address_,addr);
		strcpy(port_,port);
		// Close connection if server is changed
		if (strcmp(port_,cur_port_) !=0 || (strcmp(address_,cur_address_) != 0))
			dw_close();
	}

	// Enable the interface
	void Becker::enable(bool enable)
	{
		if (enable) {
			if (!enabled_) {
				read_pos_ = 0;
				write_pos_ = 0;

				// create thread to handle io and start it
				HANDLE hEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr ) ;
				if (hEvent==nullptr) {
					DLOG_C("Cannot create DWTCPConnection thread!\n");
					return;
				}
				unsigned threadID;
				threadHandle_ = (HANDLE) _beginthreadex
					( nullptr, 0, &Becker::threadEntry, this, 0, &threadID );

				if (threadHandle_ == nullptr) {
					DLOG_C("Cannot start DWTCPConnection thread!\n");
					return;
				}
				enabled_ = true;
				DLOG_C("DWTCPConnection thread started with id %d\n",threadID);
			}
		}  else {
			dw_close();
			enabled_ = false;
			DLOG_C("DWTCPConnection has been disabled.\n");
		}
	}

	void Becker::write(unsigned char data,unsigned short port)
	{
		if (port == 0x42)
			dw_write(data);
		return;
	}

	unsigned char Becker::read(unsigned short port)
	{
		switch (port) {
			// read status
			case 0x41:
				if (dw_status() != 0)
					return 2;
				else
					return 0;
			case 0x42:
				return(dw_read());
			default:
				return 0;
		}
	}

	// get drivewire status for status line
	void Becker::status(char* DWStatus)
	{
		if (enabled_) {
			// calculate speed
			DWORD sinceCalc = GetTickCount() - last_stats_;
			if (sinceCalc > STATS_PERIOD_MS) {
				last_stats_ += sinceCalc;
				read_speed_ = 8.0f * (bytes_read_ / (1000.0f - sinceCalc));
				write_speed_ = 8.0f * (bytes_written_ / (1000.0f - sinceCalc));
				bytes_read_ = 0;
				bytes_written_ = 0;
			}
			if (retry_) {
				sprintf(DWStatus,"DW %s?", cur_address_);
			} else if (socket_ == 0) {
				sprintf(DWStatus,"DW ConError");
			} else {
				sprintf(DWStatus,"DW OK R:%04.1f W:%04.1f", read_speed_ , write_speed_);
			}
		} else {
			sprintf(DWStatus,"");
		}
		return;
	}

	// check for input data waiting
	unsigned char Becker::dw_status()
	{
		if (retry_ || socket_ == 0 || read_pos_ == write_pos_)
			return 0;
		else
			return 1;
	}

	// coco reads a byte
	unsigned char Becker::dw_read()
	{
		// increment buffer read pos, return next byte
		unsigned char data = buffer_[read_pos_];
		read_pos_++;
		if (read_pos_ == BUFFER_SIZE) read_pos_ = 0;
		bytes_read_++;
		return data;
	}

	// coco writes a byte
	int Becker::dw_write( char data)
	{
		// send the byte if we're connected
		if ((socket_ != 0) & (!retry_)) {
			int res = send(socket_, &data, 1, 0);
			if (res != 1) {
				DLOG_C("dw_write socket error %d\n", WSAGetLastError());
				closesocket(socket_);
				socket_ = 0;
			} else {
				bytes_written_++;
        	}
		} else {
			DLOG_C("dw_write null socket\n");
		}
		return 0;
	}

	void Becker::dw_open()
	{
		// Close existing socket
		if (socket_ != 0) closesocket(socket_);
		socket_ = 0;

		// Permit retry unless urecoverable error or success
		retry_ = true;

		struct addrinfo hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family   = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		struct addrinfo *result = nullptr;
		if (getaddrinfo(cur_address_, cur_port_, &hints, &result) != 0) {
			retry_ = false;
			return;
		}

		for (auto p = result; p != nullptr; p = p->ai_next) {
			SOCKET s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
			if (s == INVALID_SOCKET) {
				retry_ = false;
				freeaddrinfo(result);
				return;
			}
			if (connect(s, p->ai_addr, p->ai_addrlen) != 0) {
				closesocket(s);
				continue;
			}
			// Connect success
			retry_ = false;
			socket_ = s;
			read_pos_ = 0;
			write_pos_ = 0;
			break;
		}
		freeaddrinfo(result);
	}

	void Becker::dw_close()
	{
		DLOG_C("dw_close\n");
		if (socket_ != 0) closesocket(socket_);
		socket_ = 0;
		read_pos_ = 0;
		write_pos_ = 0;
		threadHandle_ = nullptr;
	}

	// TCP connection thread
	unsigned __stdcall Becker::threadEntry(void* param) {
    	return static_cast<Becker*>(param)->runThread(static_cast<HANDLE>(param));
	}

	unsigned __stdcall Becker::runThread(void* /*Dummy*/)
	{
		DLOG_C("dw_thread %d\n",enabled_);
		WSADATA wsaData;

		if (enabled_) {
			// Request Winsock version 2.2
			if ((WSAStartup(0x202, &wsaData)) != 0) {
				DLOG_C("dw_thread winsock startup failed, exiting\n");
				WSACleanup();
				return 0;
			}
		}

		while(enabled_) {

			// Keep trying to connect
			dw_open();
			while (retry_ && enabled_) {
				dw_open();
				Sleep(TCP_RETRY_DELAY);
			}

			while ((socket_ != 0) && enabled_) {
				// we have a connection, read as much as possible.
				int sz;
				if (read_pos_ > write_pos_)
					sz = read_pos_ - write_pos_;
				else
					sz = BUFFER_SIZE - write_pos_;

				// read the data
				int res = recv(socket_, buffer_ + write_pos_, sz, 0); //buffer++?

				if (res < 1) {
					// no good, bail out
					closesocket(socket_);
					socket_ = 0;
				} else {
					// good recv, inc ptr
					write_pos_ += res;
					if (write_pos_ == BUFFER_SIZE) write_pos_ = 0;
				}
			}
		}

		// Not enabled - close socket if necessary
		if (socket_ != 0) closesocket(socket_);
		socket_ = 0;

		_endthreadex(0);
		return 0;
	}
}

