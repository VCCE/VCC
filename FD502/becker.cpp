//---------------------------------------------------------------------------------
// Copyright 2015 by Joseph Forgione
// This file is part of VCC (Virtual Color Computer).
// 
// VCC (Virtual Color Computer) is free software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or any later
// version.
// 
// VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
// more details.  You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see 
// <http://www.gnu.org/licenses/>.
//
//---------------------------------------------------------------------------------
//
// This code is extracted from the standalone version of becker.dll with gui
// deletions, changed external calls, and some variable name changes.
//
//---------------------------------------------------------------------------------
//#define USE_LOGGING
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include <vcc/common/logger.h>
#include "becker.h"

#define BUFFER_SIZE 512
#define TCP_RETRY_DELAY 500
#define STATS_PERIOD_MS 100

//------------------------------------------------------
// Error returns from SOCKET dw_open(...)
//------------------------------------------------------
constexpr SOCKET SOCKET_RETRY = (SOCKET)-1;
constexpr SOCKET SOCKET_FATAL = (SOCKET)-2;

//------------------------------------------------------
// local functions
//------------------------------------------------------
void dw_open();
void dw_close();
SOCKET dw_open(const char *, const char *);
unsigned char dw_status();
unsigned char dw_read();
int dw_write(char);
unsigned __stdcall dw_thread(void *);

//------------------------------------------------------
// globals
//------------------------------------------------------

static SOCKET dwSocket = 0;
static bool dwEnabled = false;
static HANDLE hDWTCPThread;

// are we retrying tcp conn
static bool retry = false;

// circular buffer for socket io
static char InBuffer[BUFFER_SIZE];
static int InReadPos = 0;
static int InWritePos = 0;

// hostname and port
static char dwaddress[MAX_PATH] = {};
static char dwsport[16] = "65504";
static char curaddress[MAX_PATH] = {};
static char curport[16] = "65504";

// statistics
static int BytesWrittenSince = 0;
static int BytesReadSince = 0;
static DWORD LastStats;
static float ReadSpeed = 0;
static float WriteSpeed = 0;

//------------------------------------------------------
// Set becker server address and port 
// Should default to "127.0.0.1" and "65504"
//------------------------------------------------------

int becker_sethost(const char *bufdwaddr, const char *bufdwport)
{
	strcpy(dwaddress,bufdwaddr);
	strcpy(dwsport,bufdwport);

	if (strcmp(dwsport,curport) !=0 || (strcmp(dwaddress,curaddress) != 0))
		dw_close();
    return 0;
}

//------------------------------------------------------
// enable or disable becker port
//------------------------------------------------------
void becker_enable(bool enable)
{
	if (enable) {
		if (!dwEnabled) {

			// reset buffer pointers
			InReadPos = 0;
			InWritePos = 0;

			// start create thread to handle io
			HANDLE hEvent;
                
			unsigned threadID;

			hEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr ) ;
                
			if (hEvent==nullptr) {
				_DLOG("Cannot create DWTCPConnection thread!\n");
				return;
			}

			// start it up...
			hDWTCPThread = (HANDLE)_beginthreadex
				( nullptr, 0, &dw_thread, hEvent, 0, &threadID );

			if (hDWTCPThread==nullptr) {
				_DLOG("Cannot start DWTCPConnection thread!\n");
				return;
			}
			dwEnabled = true;
			_DLOG("DWTCPConnection thread started with id %d\n",threadID); 
		}
	}  else {
		dw_close();
		dwEnabled = false;
		_DLOG("DWTCPConnection has been disabled.\n");
	}
}

//------------------------------------------------------
// write becker port
//------------------------------------------------------
void becker_write(unsigned char data,unsigned short port)
{
	if (port == 0x42)
		dw_write(data);
	return;
}

//------------------------------------------------------
// read becker port
//------------------------------------------------------
unsigned char becker_read(unsigned short port)
{
	switch (port) {
		// read status
		case 0x41:
			if (dw_status() != 0)
				return 2;
			else
				return 0;
		// read data 
		case 0x42:
			return(dw_read());
	}
	return 0;
}

//------------------------------------------------------
// get drivewire status for status line
//------------------------------------------------------
void becker_status(char *DWStatus)
{
	if (dwEnabled) {
		// calculate speed
		DWORD sinceCalc = GetTickCount() - LastStats;
		if (sinceCalc > STATS_PERIOD_MS) {
			LastStats += sinceCalc;
			ReadSpeed = 8.0f * (BytesReadSince / (1000.0f - sinceCalc));
			WriteSpeed = 8.0f * (BytesWrittenSince / (1000.0f - sinceCalc));
			BytesReadSince = 0;
			BytesWrittenSince = 0;
		}
		if (retry) {
			sprintf(DWStatus,"DW %s?", curaddress);
		} else if (dwSocket == 0) {
			sprintf(DWStatus,"DW ConError");
		} else {
			sprintf(DWStatus,"DW OK R:%04.1f W:%04.1f", ReadSpeed , WriteSpeed);
		}
	} else {
		sprintf(DWStatus,"");
	}
	return;
}

//------------------------------------------------------
// Internal functions
//------------------------------------------------------

// coco checks for data
unsigned char dw_status( void )
{
	// check for input data waiting
	if (retry | (dwSocket == 0) | (InReadPos == InWritePos))
		return 0;
	else
		return 1;
}

// coco reads a byte
unsigned char dw_read( void )
{
	// increment buffer read pos, return next byte
	unsigned char dwdata = InBuffer[InReadPos];
	InReadPos++;
	if (InReadPos == BUFFER_SIZE) InReadPos = 0;
	BytesReadSince++;
	return dwdata;
}

// coco writes a byte
int dw_write( char dwdata)
{
	// send the byte if we're connected
	if ((dwSocket != 0) & (!retry)) {
		int res = send(dwSocket, &dwdata, 1, 0);
		if (res != 1) {
			_DLOG("dw_write socket error %d\n", WSAGetLastError());
			closesocket(dwSocket);
			dwSocket = 0;
		} else {
			BytesWrittenSince++;
        }

	} else {
		_DLOG("dw_write null socket\n");
	}
	return 0;
}

void dw_close()
{
	// close socket to cause io thread to die
	_DLOG("dw_close\n");
	if (dwSocket != 0) closesocket(dwSocket);
	dwSocket = 0;
	InReadPos = 0;
	InWritePos = 0;
	hDWTCPThread = nullptr;
}

// try to connect with DW server
void dw_open( void )
{
	_DLOG("dw_open %s:%d\n",dwaddress,dwsport);
	dwSocket = dw_open(dwaddress,dwsport);
	if (dwSocket == SOCKET_RETRY) {
		retry = true;
		dwSocket = 0;
	} else if (dwSocket == SOCKET_FATAL) {
		retry = false;
		dwSocket = 0;
	}
}

// Override for dw_open(void) replaces gethostbyname with getaddrinfo
SOCKET dw_open(const char * server, const char * port)
{
	struct addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	struct addrinfo *result = nullptr;
	if (getaddrinfo(server, port, &hints, &result) != 0)
		return SOCKET_FATAL;

	SOCKET s = SOCKET_RETRY;
	for (auto p = result; p != nullptr; p = p->ai_next) {
		s = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (s == INVALID_SOCKET) {
			freeaddrinfo(result);
			return SOCKET_FATAL;
		}
		if (connect(s, p->ai_addr, p->ai_addrlen) != 0) {
			closesocket(s);
			s = SOCKET_RETRY;
			continue;
		}
		break;
	}
	freeaddrinfo(result);
	return s;
}

// TCP connection thread
unsigned __stdcall dw_thread(void* /*Dummy*/)
{
	_DLOG("dw_thread %d\n",dwEnabled);
	WSADATA wsaData;
        
	int sz;
	int res;

	if (dwEnabled) {
		// Request Winsock version 2.2
		if ((WSAStartup(0x202, &wsaData)) != 0) {
			_DLOG("dw_thread winsock startup failed, exiting\n");
			WSACleanup();
			return 0;
		}
	}
	while(dwEnabled) {
		// get connected
		dw_open();

		// keep trying...
		while ((dwSocket == 0) & dwEnabled) {
			dw_open();
			// after 2 tries, sleep between attempts
			Sleep(TCP_RETRY_DELAY);
		}
                
		while ((dwSocket != 0) & dwEnabled) {
			// we have a connection, read as much as possible.
			if (InReadPos > InWritePos)
				sz = InReadPos - InWritePos;
			else
				sz = BUFFER_SIZE - InWritePos;

			// read the data
			res = recv(dwSocket,(char *)InBuffer + InWritePos, sz, 0);

			if (res < 1) {
				// no good, bail out
				closesocket(dwSocket);
				dwSocket = 0;
			} else {
				// good recv, inc ptr
				InWritePos += res;
				if (InWritePos == BUFFER_SIZE) InWritePos = 0;
			}
		}
	}

	// Not enabled - close socket if necessary
	if (dwSocket != 0) closesocket(dwSocket);
                
	dwSocket = 0;

	_endthreadex(0);
	return 0;
}
