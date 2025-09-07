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
// Input from or Output to windows com port
//------------------------------------------------------------------

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "acia.h"
//#include "sc6551.h"
#include "../logger.h"

HANDLE hReadEvent;
HANDLE hWriteEvent;
HANDLE hComPort=INVALID_HANDLE_VALUE;

int writeport(char *buf,int siz);

static unsigned int BaudRate;
static unsigned int EnParity;
static unsigned int Parity;
static unsigned int DataLen;
static unsigned int StopBits;

//=====================================================
// Open COM port. Port I/O must be asyncronous (overlapped)
// otherwise pending reads will block pending writes
//=====================================================
int wincom_open()
{
    DCB PortDCB;
    static int bt[16]={38400, 110, 110, 110, 300, 300, 300,  600,
                        1200,2400,2400,4800,4800,9600,9600,19200};

    hReadEvent  = CreateEvent(nullptr,TRUE,FALSE,nullptr);
    hWriteEvent = CreateEvent(nullptr,TRUE,FALSE,nullptr);

    char portname[64];
    sprintf(portname,"\\\\.\\%s",AciaComPort);
    hComPort = CreateFileA(portname,
                           GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           OPEN_EXISTING, FILE_FLAG_OVERLAPPED,nullptr);
    if (hComPort==INVALID_HANDLE_VALUE) return -1; // msgbox?

    // Port settings
    SecureZeroMemory(&PortDCB,sizeof(PortDCB));
    PortDCB.DCBlength = sizeof(PortDCB);
    GetCommState(hComPort,&PortDCB);
    PortDCB.BaudRate = (BaudRate > 16) ? 9600 : bt[BaudRate];
    PortDCB.fParity  = EnParity;
    PortDCB.Parity   = (EnParity) ? 0 : Parity;
    PortDCB.ByteSize = DataLen;
    PortDCB.StopBits = (StopBits == 0) ? 0 : 2;
    SetCommState(hComPort,&PortDCB);
    // PrintLogF("Baud:%d,Parity:%d,Data:%d,Stop:%d\n",
    //           BaudRate,Parity,DataLen,StopBits);
    return 0;
}

//=====================================================
// Close COM port
//=====================================================
void wincom_close()
{
    if (hComPort) CloseHandle(hComPort);
    hComPort = INVALID_HANDLE_VALUE;
}

//=====================================================
// Read from com port.  If text remove LF characters
//=====================================================

int wincom_read(char* buf,int siz)
{
    int rc;
    DWORD cnt = 0;
    if (hComPort == INVALID_HANDLE_VALUE) return -1;

    OVERLAPPED read_ovl = {0};
    read_ovl.hEvent = hReadEvent;

    // Read at least one char
    while (cnt < 1) {
        rc = ReadFile(hComPort,buf,siz,&cnt,&read_ovl);
        if (rc == 0) {
            if (GetLastError() != ERROR_IO_PENDING) return -1;
            WaitForSingleObject(hReadEvent,INFINITE);
            rc = GetOverlappedResult(hComPort, &read_ovl, &cnt, TRUE);
            if (cnt < 0) return -1;
        }
    }
    // If text mode convert every LF to CR
    if (AciaTextMode) {
        for (unsigned int i=0;i<cnt;i++) if (buf[i]==10) buf[i]=13;
    }
    return cnt;
}

//=====================================================
// Write to com port.  If text mode convert line endings
//=====================================================
int  wincom_write(char* buf,int siz)
{
    int cnt = 0;
    if (hComPort == INVALID_HANDLE_VALUE) return -1;

    if (AciaTextMode) {
        // Convert endings. Skip LF and convert CR to CRLF.
        // writeport is much faster with large writes. Code
        // here maximizes write sizes without re-buffering.
        while(cnt < siz) {
            // search for a CR or LF
            char *ptr, *tbuf, chr;
            int tsiz;
            ptr = tbuf = buf + cnt;
            for (tsiz = 0; tsiz < siz-cnt; tsiz++) {
                chr = *ptr++;
                if (chr == '\r' || chr == '\n') break;
            }
            // write buffer up to but excluding the chr found
            if (tsiz) {
                int len = writeport(tbuf,tsiz);
                if (len < 1) return -1;
                cnt += len;
                // loop if write was short
                if (len != tsiz) continue;
            }
            // Skip LF or convert CR to CRLF
            switch (chr) {
            case '\r':  // CR
                writeport("\r\n",2);
            case '\n':  // LF
                cnt++;
            }
        }
    }  else {
        cnt = writeport(buf,siz);
    }
    return cnt;
}

//=====================================================
// Write function for overlapped I/O
//=====================================================
int writeport(char *buf,int siz) {
    OVERLAPPED ovl = {0};
    ovl.hEvent = hWriteEvent;
    DWORD cnt;
    int rc;
    rc = WriteFile(hComPort,buf,siz,&cnt,&ovl);
    if (rc == 0) {
        if (GetLastError() != ERROR_IO_PENDING) return -1;
        WaitForSingleObject(hWriteEvent,INFINITE);
        GetOverlappedResult(hComPort, &ovl, &cnt, TRUE);
    }
    return cnt;
}

