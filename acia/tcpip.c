#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include "acia.h"
#include "logger.h"

// socket
static SOCKET Socket = INVALID_SOCKET;

//Close
void tcpip_close()
{
    if (Socket != INVALID_SOCKET) closesocket(Socket);
    Socket = 0;
    WSACleanup();
}

// Open
int tcpip_open()
{
    WSADATA wsaData;

    if ((WSAStartup(0x202, &wsaData)) != 0) {
        WSACleanup();
        return -1;
    }

    // resolve hostname
    LPHOSTENT host = gethostbyname(SrvAddress);
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
    addr.sin_port = htons(SrvPort);

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
    while (cnt < 1) {
        if (Socket == INVALID_SOCKET) return 0; //tcpip_open();
        cnt = recv(Socket,buf,siz,0);
        if (cnt < 1) Sleep(100);
    }

    // Fix line endings
    for (int i=0;i<cnt;i++) if (buf[i]==10) buf[i]=13;

//for (int i=0;i<cnt;i++) {
//  if (buf[i] >= ' ')
//    PrintLogF("%c",buf[i]);
//  else
//    PrintLogF("\\x%02X ",buf[i]);
//}
    return cnt;
}

//Write
int tcpip_write(char* buf, int siz)
{
    if (Socket == INVALID_SOCKET) return -1; //tcpip_open();
    int cnt = 0;
    while (siz-- > 0) {
        send(Socket,buf++,1,0);
        cnt++;
    }
//TODO fix line endings
	return cnt;
}

