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

#include "Socket.h"

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>

#define F(x) #x,x

namespace VCC
{
    namespace
    {
        // flags for socket send/recv
        static constexpr auto sendFlags = 0; // unix: MSG_NOSIGNAL 
        static constexpr auto recvFlags = 0;
    }

    Socket::WSAStartup_t Socket::WSAStartup = nullptr;
    Socket::WSACleanup_t Socket::WSACleanup = nullptr;
    Socket::WSAGetLastError_t Socket::WSAGetLastError = nullptr;
    Socket::socket_t Socket::socket = nullptr;
    Socket::connect_t Socket::connect = nullptr;
    Socket::closesocket_t Socket::closesocket = nullptr;
    Socket::htons_t Socket::htons = nullptr;
    Socket::send_t Socket::send = nullptr;
    Socket::recv_t Socket::recv = nullptr;
    Socket::inet_addr_t Socket::inet_addr = nullptr;
    Socket::gethostbyname_t Socket::gethostbyname = nullptr;
    Socket::setsockopt_t Socket::setsockopt = nullptr;
    Socket::bind_t Socket::bind = nullptr;
    Socket::listen_t Socket::listen = nullptr;
    Socket::accept_t Socket::accept = nullptr;
    Socket::WSAPoll_t Socket::WSAPoll = nullptr;

    HMODULE Socket::hWs2 = nullptr;
    bool Socket::initialized = false;
    char Socket::err[100];

    //////////////////////////////////////////////////////////////////////////
    //
    // Private wrapper on result code for logging errors
    //
    static int Result(int code)
    {
        if (code != Socket::OK)
        {
            char message[128];
            snprintf(message, sizeof(message), "socket error %d: %s\n", code, Socket::LastError());
            OutputDebugString(message);
        }
        return code;
    }

    //////////////////////////////////////////////////////////////////////////

    Socket::Socket()
        : mSock(INVALID_SOCKET)
        , mConnected(false)
    {
    }

    //////////////////////////////////////////////////////////////////////////

    Socket::Socket(int handle)
        : mSock(handle)
        , mConnected(true)
    {

    }

    //////////////////////////////////////////////////////////////////////////

    Socket::Socket(Socket&& o) noexcept 
        : mSock(o.mSock)
        , mConnected(o.mConnected)
    {
        o.Reset();
    }

    //////////////////////////////////////////////////////////////////////////

    Socket& Socket::operator=(Socket&& o) noexcept
    {
        if (IsOpen()) Close();
        mSock = o.mSock;
        mConnected = o.mConnected;
        o.Reset();
        return *this;
    }

    //////////////////////////////////////////////////////////////////////////

    const char* Socket::LastError()
    {
        return err;
    }

    //////////////////////////////////////////////////////////////////////////

    bool Socket::IsConnected() const
    {
        return mConnected;
    }

    //////////////////////////////////////////////////////////////////////////

    bool Socket::IsOpen() const
    {
        return mSock != INVALID_SOCKET;
    }

    //////////////////////////////////////////////////////////////////////////

    int Socket::Init()
    {
        // already initialized dont do it again
        if (hWs2) return Result(OK);

        // load winsock library
        hWs2 = LoadLibraryA("ws2_32.dll");
        if (!hWs2)
        {
            sprintf(err, "unable to load winsock dll err = %d", GetLastError());
            return Result(ERR_MISSING_DLL);
        }

        // gather apis
        if (!GetApi(F(WSAStartup))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(WSACleanup))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(WSAGetLastError))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(socket))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(connect))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(closesocket))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(htons))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(send))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(recv))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(inet_addr))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(gethostbyname))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(setsockopt))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(bind))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(listen))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(accept))) return Result(ERR_MISSING_FUNC);
        if (!GetApi(F(WSAPoll))) return Result(ERR_MISSING_FUNC);

        WSADATA wsadata;
        if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0)
        {
            sprintf(err, "unable to start winsock err = %d", WSAGetLastError());
            Shutdown();
            return Result(ERR_CANT_START);
        }

        initialized = true;
        return Result(OK);
    }

    //////////////////////////////////////////////////////////////////////////

    int Socket::Shutdown()
    {
        if (hWs2)
            FreeLibrary(hWs2);
        hWs2 = nullptr;
        return Result(OK);
    }

    ////////////////////////////////////////////////////////////////////////////

    int Socket::Open(const char* address, int port)
    {
        if (!initialized)
            return Result(ERR_NOT_INITIALIZED);

        mSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (mSock == INVALID_SOCKET)
            return Result(ERR_INVALID_SOCKET);

        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.S_un.S_addr = inet_addr(address);

        if (sa.sin_addr.S_un.S_addr == INADDR_NONE)
        {
            hostent* host = gethostbyname(address);
            if (!host)
            {
                sprintf(err, "unable to resolve address %s", address);
                return Result(ERR_CANT_RESOLVE);
            }

            memcpy(&sa.sin_addr, host->h_addr_list[0], host->h_length);
        }

        if (connect(mSock, (sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR)
        {
            sprintf(err, "connect failed %d", WSAGetLastError());
            return Result(ERR_CANT_CONNECT);
        }

        mConnected = true;

        return Result(OK);
    }

    int Socket::Listen(const char* address, int port)
    {
        if (!initialized)
            return Result(ERR_NOT_INITIALIZED);

        mSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (mSock == INVALID_SOCKET)
            return Result(ERR_INVALID_SOCKET);

        int opt = 1;
        setsockopt(mSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.S_un.S_addr = inet_addr(address);

        if (sa.sin_addr.S_un.S_addr == INADDR_NONE)
        {
            hostent* host = gethostbyname(address);
            if (!host)
            {
                sprintf(err, "unable to resolve address %s", address);
                return Result(ERR_CANT_RESOLVE);
            }

            memcpy(&sa.sin_addr, host->h_addr_list[0], host->h_length);
        }

        if (bind(mSock, (sockaddr*)&sa, sizeof(sa)) < 0) 
        {
            Close();
            return Result(ERR_BIND_FAILED);
        }

        if (listen(mSock, 1) < 0) 
        {
            Close();
            return Result(ERR_LISTEN_FAILED);
        }
        return Result(OK);
    }

    //////////////////////////////////////////////////////////////////////////

    int Socket::Poll(int timeoutMs)
    {
        pollfd pdf{};
        pdf.fd = mSock;
        pdf.events = POLLIN;

        int result = WSAPoll(&pdf, 1, timeoutMs);
        if (result < 0) return Result(ERR_ACCEPT_FAIL);
        if (result == 0) return ERR_TIMEDOUT;
        return Result(OK);
    }

    //////////////////////////////////////////////////////////////////////////

    int Socket::Accept(Socket& out, int timeoutMs)
    {
        auto p = Poll(timeoutMs);
        if (p != OK) return p;
        int client = accept(mSock, nullptr, nullptr);
        if (client < 0) return Result(ERR_ACCEPT_FAIL);
        out = Socket(client);
        return Result(OK);
    }

    //////////////////////////////////////////////////////////////////////////

    int Socket::Close()
    {
        if (!initialized)
            return Result(ERR_NOT_INITIALIZED);

        if (!IsOpen())
            return Result(ERR_NOT_OPEN);

        closesocket(mSock);
        mSock = INVALID_SOCKET;
        mConnected = false;

        return Result(OK);
    }

    //////////////////////////////////////////////////////////////////////////

    int Socket::Send(const char* buffer, int bufferLen, int *bytesSent) const
    {
        if (!IsOpen()) 
            return Result(ERR_NOT_OPEN);
        int bytes = send(mSock, buffer, bufferLen, sendFlags);
        if (bytes < 0) 
            return Result(ERR_SEND_FAIL);
        if (bytesSent) *bytesSent = bytes;
        return Result(OK);
    }

    //////////////////////////////////////////////////////////////////////////

    int Socket::Recv(char* buffer, int bufferLen, int *bytesRecv)
    {
        if (!IsOpen()) 
            return Result(ERR_NOT_OPEN);
        int bytes = recv(mSock, buffer, bufferLen, recvFlags);
        if (bytes < 0) 
            return Result(ERR_RECV_FAIL);
        if (bytesRecv) *bytesRecv = bytes;
        return Result(OK);
    }

    //////////////////////////////////////////////////////////////////////////

    void Socket::Reset()
    {
        mSock = INVALID_SOCKET;
        mConnected = false;
    }
}

