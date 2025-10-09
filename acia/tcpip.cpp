//------------------------------------------------------------------
// Copyright E J Jaquay 2022
//
// This file is part of VCC (Virtual Color Computer).
//
// VCC (Virtual Color Computer) is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details.  You should have
// received a copy of the GNU General Public License along with VCC
// (Virtual Color Computer). If not see <http://www.gnu.org/licenses/>.
//
//------------------------------------------------------------------

//------------------------------------------------------------------
// Input from or Output to tcpip server
//------------------------------------------------------------------

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include "acia.h"
#include <vcc/common/logger.h>

// socket
static SOCKET Socket = INVALID_SOCKET;

//Close
void tcpip_close()
{
    _DLOG("tcpip_close Socket %d\n",Socket);
    if (Socket != INVALID_SOCKET) closesocket(Socket);
    Socket = INVALID_SOCKET;
    WSACleanup();
}

// Open
int tcpip_open()
{
    WSADATA wsaData;
    if (Socket != INVALID_SOCKET) tcpip_close();

    if ((WSAStartup(0x202, &wsaData)) != 0) {
        WSACleanup();
        return -1;
    }
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *result = nullptr;
    if (getaddrinfo(AciaTcpHost, AciaTcpPort, &hints, &result) != 0) {
    _DLOG("tcpip_open getaddrinfo fail %s %s\n",AciaTcpHost,AciaTcpPort);
    return -1;
}

    for (auto p = result; p != nullptr; p = p->ai_next) {
        Socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (Socket == INVALID_SOCKET) {
            freeaddrinfo(result);
            _DLOG("tcpip_open Fail\n");
            return -1;
        }
        if (connect(Socket, p->ai_addr, p->ai_addrlen) != 0) {
            closesocket(Socket);
            Socket = INVALID_SOCKET;
            continue;
        }
        break;
    }
    freeaddrinfo(result);

    _DLOG("tcpip_open Socket %d\n",Socket);

    if (Socket == INVALID_SOCKET)
        return -1;
    else
        return 0;
}

//Read
int tcpip_read(char* buf, int siz)
{
    int cnt = 0;
    if (Socket == INVALID_SOCKET) tcpip_open();

    while (cnt < 1) {
        cnt = recv(Socket,buf,siz,0);
        if (AciaTextMode && (cnt == 0)) {
            buf[cnt++] = EOFCHR;
            break;
        }
        if (cnt < 1) Sleep(100);
    }

    // Fix line endings
    if (AciaTextMode) {
        for (int i=0;i<cnt;i++) if (buf[i]==10) buf[i]=13;
    }
    return cnt;
}

//Write
int tcpip_write(const char* buf, int siz)
{
    char CRLF[3]="\r\n";

    int cnt = 0;
    int rc;

    if (Socket == INVALID_SOCKET) tcpip_open();
    while (siz-- > 0) {
        if (AciaTextMode) {
            if (*buf != '\n') {
                rc = send(Socket,buf,1,0);
                if (rc != 1) break;
            }
            if (*buf == '\r') {
                rc = send(Socket,CRLF,2,0);
                if (rc != 2) break;
            }
        } else {
            rc = send(Socket,buf,1,0);
            if (rc != 1) break;
        }
        buf++;
        cnt++;
    }
    return cnt;
}

