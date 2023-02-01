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
#include "logger.h"

HANDLE hReadEvent;
HANDLE hWriteEvent;
HANDLE hComPort=INVALID_HANDLE_VALUE;

//=====================================================
int wincom_open()
{
    DCB PortDCB;
    COMMTIMEOUTS cto = {10,10,10,10,10};

	hReadEvent  = CreateEvent(NULL,TRUE,FALSE,NULL);
	hWriteEvent = CreateEvent(NULL,TRUE,FALSE,NULL);

PrintLogF("Attempting Open\n");
    // COM ports require overlapped I/O
	hComPort = CreateFileA("\\\\.\\COM20",
						   GENERIC_READ | GENERIC_WRITE, 0, NULL,
						   OPEN_EXISTING, FILE_FLAG_OVERLAPPED,NULL);
    if (hComPort==INVALID_HANDLE_VALUE) {
PrintLogF("Open Fail\n");
		return -1;
	}
PrintLogF("Opened\n");

    // Get settings
    SecureZeroMemory(&PortDCB,sizeof(PortDCB));
    PortDCB.DCBlength = sizeof(PortDCB);
	GetCommState(hComPort,&PortDCB);

PrintLogF("Open Baud %d, Bits %d \n", PortDCB.BaudRate,PortDCB.ByteSize);

    cto.ReadIntervalTimeout = 100;

    //Change stuff here

    //SetCommState(hComPort,&PortDCB);
	//SetCommTimeouts(mPortFh, &cto);

	return 0;
}

//=====================================================
void wincom_close()
{
    if(hComPort) CloseHandle(hComPort);
    hComPort = INVALID_HANDLE_VALUE;
}

//=====================================================
// Read from com port.  If text remove LF characters
// Port I/O must be asyncronous (overlapped)
// otherwise pending reads will block pending writes

int wincom_read(char* buf,int siz)
{
	if (hComPort == INVALID_HANDLE_VALUE) return -1;
	OVERLAPPED read_ovl = {0};
    read_ovl.hEvent = hReadEvent;

    int rc;
    int cnt=0;

	while (cnt < 1) {
        rc = ReadFile(hComPort,buf,siz,&cnt,&read_ovl);
        if (rc == 0) {
		    if (GetLastError() != ERROR_IO_PENDING) return -1; 
			WaitForSingleObject(hReadEvent,INFINITE);
	        rc = GetOverlappedResult(hComPort, &read_ovl, &cnt, TRUE);
		    if (cnt < 0) return -1;
        }
		// Add EOF char if endof file
        if ((AciaTextMode) && (cnt == 0)) {
            buf[cnt++] = EOFCHR;
            break;
        }
    }
    // Fix line endings
    if (AciaTextMode) {
        for (int i=0;i<cnt;i++) if (buf[i]==10) buf[i]=13;
    }
    return cnt;
}

//=====================================================
// Write to com port.  If text mode convert line endings
//=====================================================
int writeport(char *buf,int siz) {
	OVERLAPPED ovl = {0};
    ovl.hEvent = hWriteEvent;
    int cnt;
    int rc;
	rc = WriteFile(hComPort,buf,siz,&cnt,&ovl);
    if (rc == 0) {
		if (GetLastError() != ERROR_IO_PENDING) return -1;
		WaitForSingleObject(hWriteEvent,INFINITE);
	    GetOverlappedResult(hComPort, &ovl, &cnt, TRUE);
    }
	return cnt;
}
int  wincom_write(char* buf,int siz)
{
    //PrintLogF("W %d",siz);
	int cnt = 0;
	if (hComPort == INVALID_HANDLE_VALUE) return -1;

    if (AciaTextMode) {
        // Text mode convert endings. Simpler code to write one char 
	    // at a time or rebuffer but the following avoids one char
	    // writes without rebuffering.
        while(cnt < siz) {
			// look for a line ending
			char *tbuf = buf + cnt;
		    int ending = 0;
			char *ptr = tbuf;
			int tsiz;
			for (tsiz=0; tsiz < siz-cnt; tsiz++) {
		        if (*ptr=='\r') ending = 1;  // CR
		        if (*ptr=='\n') ending = 2;  // LF
				if (ending) break;
				ptr++;
			}
			// write buffer up to line ending
			if (tsiz > 0) {
		        int len = writeport(tbuf,tsiz);
		        if (len < 1) return -1;
			    cnt += len;
			    // try again if write was short
			    if (len != tsiz) continue;
			}
			// Skip LF, write CRLF if CR
			if (ending) {
				cnt++;
			    if (ending == 1) 
					if (writeport("\r\n",2) != 2) return -1;
			}
        }
	}  else {
		return writeport(buf,siz);
	}
    //PrintLogF(" %d\n",cnt);
    return cnt;
}

