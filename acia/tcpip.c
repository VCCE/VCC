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

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "acia.h"
#include "../logger.h"

// socket
static SOCKET Socket = INVALID_SOCKET;

//Close
void tcpip_close()
{
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

    // resolve hostname
    LPHOSTENT host = gethostbyname(AciaTcpHost);
    if (host == NULL) {
        tcpip_close();
        return -1;
    }

    // allocate socket
    Socket = socket (AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (Socket == INVALID_SOCKET) {
        tcpip_close();
        return -1;
    }

    BOOL bTrue = TRUE;
    int iTrue = 1;
    setsockopt(Socket,IPPROTO_TCP,
                SO_REUSEADDR,(const char *)&bTrue,sizeof(bTrue));
    setsockopt(Socket,IPPROTO_TCP,
                TCP_NODELAY,(const char *)&iTrue,sizeof(iTrue));

    // try to connect...
    SOCKADDR_IN addr;
    addr.sin_family= AF_INET;
    addr.sin_addr= *((LPIN_ADDR)*host->h_addr_list);
    addr.sin_port = htons(AciaTcpPort);
    int rc = connect(Socket,(LPSOCKADDR)&addr, sizeof(addr));

    if (rc==SOCKET_ERROR) {
        tcpip_close();
        return -1;
   }
    return(0);
}

//Read
int tcpip_read(char* buf, int siz)
{
    int cnt = 0;
    if (Socket == INVALID_SOCKET) tcpip_open();

    while (cnt < 1) {
        cnt = recv(Socket,buf,siz,0);
        if ((AciaTextMode) && (cnt == 0)) {
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
int tcpip_write(char* buf, int siz)
{
    char CRLF[2]="\r\n";

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

