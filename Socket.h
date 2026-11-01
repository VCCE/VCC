/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.

    Debug Server DAP - Craig Allsop 2026
*/

#pragma once

#include "BuildConfig.h"

//
// begin winsock types (avoid requiring header)
//
#define FDECLARE_HANDLE(name)  typedef struct name##__ *name
typedef void* HANDLE;
FDECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
struct WSAData;
typedef WSAData* LPWSADATA;
struct pollfd;
typedef pollfd* PWSAPOLLFD;
typedef unsigned short WORD;
typedef unsigned int SOCKET;
typedef unsigned short u_short;
struct hostent;
struct sockaddr;
//
// end winsock types
//

namespace VCC
{
    //
    // Socket, just your plain old tcp socket
    //
    struct Socket
    {
        //
        // Initialization for socket usage in program
        //
        static int Init();

        //
        // Shutdown sockets
        //
        static int Shutdown();

        Socket();
        Socket(const Socket&) = delete;
        Socket(Socket&& o) noexcept;

        //
        // assignment
        //
        Socket& operator=(const Socket&) = delete;
        Socket& operator=(Socket&& o) noexcept;

        //
        // Open connection to address at port
        //
        int Open(const char* address, int port);

        //
        // Open port to listen from address at port
        //
        int Listen(const char* address, int port);

        //
        // Accept new connection
        //
        int Accept(Socket& out, int timeoutMs);

        //
        // Poll for input, returns ERR_TIMEDOUT on timeout.
        //
        int Poll(int timeoutMs);

        //
        // Return true if socket handle is open
        //
        bool IsOpen() const;

        //
        // Returns true when sucessfully connected
        //
        bool IsConnected() const;

        //
        // Close this connection
        //
        int Close();

        //
        // Send buffer into socket
        // ERR_SEND_FAIL - send failed, peer disconnect
        //
        int Send(const char* buffer, int bufferLen, int* bytesSent = nullptr) const;

        //
        // Recieve waiting data from socket
        // ERR_NOT_OPEN - socket not opened
        // ERR_RECV_FAIL - recv failed, peer disconnect
        //
        int Recv(char* buffer, int bufferLen, int* bytesRecv = nullptr);

        //
        // Fetch last error message
        //
        static const char* LastError();

        enum
        {
            OK = 0,
            ERR_MISSING_DLL = 100,  // dll not available
            ERR_MISSING_FUNC,       // api function not available
            ERR_NOT_INITIALIZED,    // Init hasn't been called
            ERR_INVALID_SOCKET,     // default error state
            ERR_CANT_START,         // can't start winsock
            ERR_CANT_RESOLVE,       // couldn't resolve address
            ERR_CANT_CONNECT,       // couldn't connect to end point
            ERR_NOT_OPEN,           // socket not opened
            ERR_LISTEN_FAILED,      // listen failed
            ERR_BIND_FAILED,        // bind failed
            ERR_ACCEPT_FAIL,        // accept failed
            ERR_SEND_FAIL,          // a send failed
            ERR_RECV_FAIL,          // a recv failed
            ERR_TIMEDOUT,           // poll timed out
        };

    private:

        //
        // Api function types
        //
        typedef int(__stdcall* WSAStartup_t)(WORD, LPWSADATA);
        typedef int(__stdcall* WSACleanup_t)();
        typedef int(__stdcall* WSAGetLastError_t)();
        typedef SOCKET(__stdcall* socket_t)(int, int, int);
        typedef int(__stdcall* closesocket_t)(SOCKET);
        typedef u_short(__stdcall* htons_t)(u_short);
        typedef int(__stdcall* connect_t)(SOCKET, const sockaddr*, int);
        typedef int(__stdcall* send_t)(SOCKET, const char*, int, int);
        typedef int(__stdcall* recv_t)(SOCKET, char*, int, int);
        typedef unsigned long(__stdcall* inet_addr_t)(const char*);
        typedef hostent* (__stdcall* gethostbyname_t)(const char*);
        typedef int(__stdcall* setsockopt_t)(SOCKET, int, int, const char*, int);
        typedef int(__stdcall* bind_t)(SOCKET, const sockaddr*, int);
        typedef int(__stdcall* listen_t)(SOCKET, int);
        typedef SOCKET(__stdcall* accept_t)(SOCKET, sockaddr*, int*);
        typedef int(__stdcall* WSAPoll_t)(PWSAPOLLFD, unsigned long, int);

        //
        // Winsock Apis dynamically loaded
        //
        static bool initialized;
        static WSAStartup_t WSAStartup;
        static WSACleanup_t WSACleanup;
        static WSAGetLastError_t WSAGetLastError;
        static socket_t socket;
        static connect_t connect;
        static closesocket_t closesocket;
        static htons_t htons;
        static send_t send;
        static recv_t recv;
        static inet_addr_t inet_addr;
        static gethostbyname_t gethostbyname;
        static setsockopt_t setsockopt;
        static bind_t bind;
        static listen_t listen;
        static accept_t accept;
        static WSAPoll_t WSAPoll;

        static HMODULE hWs2;
        static char err[100];

        //
        // Locals
        //
        SOCKET mSock;
        bool mConnected;

        // for accept 
        Socket(int handle);

        //
        // Lookup api on winsock dll
        //
        template <typename T>
        static bool GetApi(const char* name, T& func)
        {
            func = (T)(GetProcAddress(hWs2, name));
            if (!func)
                sprintf(err, "unable to lookup function %s", name);
            return !!func;
        }

        //
        // reset to default
        //
        void Reset();
    };
}

