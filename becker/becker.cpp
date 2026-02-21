//#define USE_LOGGING
//======================================================================
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
//======================================================================

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include "becker.h"
#include "resource.h" 
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/util/limits.h>
#include <vcc/util/logger.h>
#include <vcc/util/FileOps.h>
#include <vcc/util/DialogOps.h>
#include <vcc/bus/cartridge_menu.h>
#include <vcc/util/settings.h>

// socket
static SOCKET dwSocket = 0;

// vcc stuff
static HINSTANCE gModuleInstance;
slot_id_type gSlotId {}; 
static PakAssertCartridgeLineHostCallback PakSetCart = nullptr;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
static unsigned char HDBRom[8192];
static bool DWTCPEnabled = false;

static HWND g_hConfigDlg;

// are we retrying tcp conn
static bool retry = false;

// circular buffer for socket io
static char InBuffer[BUFFER_SIZE];
static int InReadPos = 0;
static int InWritePos = 0;

// statistics
static int BytesWrittenSince = 0;
static int BytesReadSince = 0;
static DWORD LastStats;
static float ReadSpeed = 0;
static float WriteSpeed = 0;

// hostname and port

static char dwaddress[MAX_PATH];
static unsigned short dwsport = 65504;
static char curaddress[MAX_PATH];
static unsigned short curport = 65504;

//thread handle
static HANDLE hDWTCPThread;

// scratchpad for msgs
char msg[MAX_PATH];

// log lots of stuff...
static boolean logging = false;

//unsigned char LoadExtRom(const char *);
void SetDWTCPConnectionEnable(unsigned int enable);
int dw_setaddr(const char *bufdwaddr);
int dw_setport(const char *bufdwport);
void LoadConfig();
void SaveConfig();

static VCC::Bus::cartridge_menu BeckerMenu {};
bool get_menu_item(menu_item_entry* item, size_t index);

// Access the settings object
static VCC::Util::settings* gpSettings = nullptr;
VCC::Util::settings& Setting() {return *gpSettings;}

// dll entry hook
BOOL APIENTRY DllMain( HINSTANCE  hinstDLL, 
                       DWORD  reason, 
                       LPVOID lpReserved
					 )
{
    switch (reason)
	{
		case DLL_PROCESS_ATTACH:
			// init
			gModuleInstance = hinstDLL;
			break;

		case DLL_PROCESS_DETACH:
			// Clean up here 
			CloseCartDialog(g_hConfigDlg);
			break;

		// not used by Vcc
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
    }

    return TRUE;
}

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

        if (InReadPos == BUFFER_SIZE)
                InReadPos = 0;

        BytesReadSince++;

        return dwdata;
}


// coco writes a byte
int dw_write( char dwdata)
{

        // send the byte if we're connected
        if ((dwSocket != 0) & (!retry))
        {	
				int res = send(dwSocket, &dwdata, 1, 0);
                if (res != 1)
                {
                     DLOG_C("dw_write: socket error %d\n", WSAGetLastError());
                     closesocket(dwSocket);
                     dwSocket = 0;
                }
                else
                {
                        BytesWrittenSince++;
                }
        }
	     else
	    {
                DLOG_C("coco write but null socket error\n");
	     }

        return 0;
}


void killDWTCPThread()
{

        // close socket to cause io thread to die
        if (dwSocket != 0)
                closesocket(dwSocket);

        dwSocket = 0;
        
        // reset buffer po
        InReadPos = 0;
        InWritePos = 0;

}




// set our hostname, called from config.c
int dw_setaddr(const char *bufdwaddr)
{
        strcpy(dwaddress,bufdwaddr);
        return 0;
}


// set our port, called from config.c
int dw_setport(const char *bufdwport)
{
        dwsport = (unsigned short)atoi(bufdwport);

        if ((dwsport != curport) || (strcmp(dwaddress,curaddress) != 0))
        {
                // host or port has changed, kill open connection
                killDWTCPThread();
        }

        return 0;
}



// try to connect with DW server
void attemptDWConnection( void )
{

        retry = true;
        BOOL bOptValTrue = TRUE;
        int iOptValTrue = 1;


        strcpy(curaddress, dwaddress);
        curport= dwsport;

        // resolve hostname
        LPHOSTENT dwSrvHost= gethostbyname(dwaddress);
        
        if (dwSrvHost == NULL)
        {
                // invalid hostname/no dns
                retry = false;
        }
        
        // allocate socket
        dwSocket = socket (AF_INET,SOCK_STREAM,IPPROTO_TCP);

        if (dwSocket == INVALID_SOCKET)
        {
              // no deal
              retry = false;
              DLOG_C("invalid socket.\n");
        }

        // set options
        setsockopt(dwSocket,IPPROTO_TCP,SO_REUSEADDR,(char *)&bOptValTrue,sizeof(bOptValTrue));
        setsockopt(dwSocket,IPPROTO_TCP,TCP_NODELAY,(char *)&iOptValTrue,sizeof(iOptValTrue));  

        // build server address
        SOCKADDR_IN dwSrvAddress;

        dwSrvAddress.sin_family= AF_INET;
        dwSrvAddress.sin_addr= *((LPIN_ADDR)*dwSrvHost->h_addr_list);
        dwSrvAddress.sin_port = htons(dwsport);
        
        // try to connect...
        int rc = connect(dwSocket,(LPSOCKADDR)&dwSrvAddress, sizeof(dwSrvAddress));

        retry = false;

        if (rc==SOCKET_ERROR)
        {
                // no deal
                closesocket(dwSocket);
                dwSocket = 0;
        }
}


// TCP connection thread
unsigned __stdcall DWTCPThread(void *Dummy)
{
        HANDLE hEvent = (HANDLE)Dummy;
        WSADATA wsaData;
        
        int sz;
        int res;

         // Request Winsock version 2.2
        if ((WSAStartup(0x202, &wsaData)) != 0)
        {
                DLOG_C("WSAStartup() failed, DWTCPConnection thread exiting\n");
                WSACleanup();
                return 0;
        }
        
        

        while(DWTCPEnabled)
        {
                // get connected
                attemptDWConnection();

                // keep trying...
                while ((dwSocket == 0) & DWTCPEnabled)
                {
                        attemptDWConnection();

                        // after 2 tries, sleep between attempts
                        Sleep(TCP_RETRY_DELAY);
                }
                
                while ((dwSocket != 0) & DWTCPEnabled)
                {
                        // we have a connection, lets chew through some i/o
                        
                        // always read as much as possible, 
                        // max read is writepos to readpos or buffersize
                        // depending on positions of read and write ptrs

                        if (InReadPos > InWritePos)
                                sz = InReadPos - InWritePos;
                        else
                                sz = BUFFER_SIZE - InWritePos;

                        // read the data
                        res = recv(dwSocket,(char *)InBuffer + InWritePos, sz, 0);

                        if (res < 1)
                        {
                                // no good, bail out
                                closesocket(dwSocket);
                                dwSocket = 0;
                        } 
                        else
                        {
                                // good recv, inc ptr
                                InWritePos += res;
                                if (InWritePos == BUFFER_SIZE)
                                        InWritePos = 0;
                                        
                        }

                }

        }

        // close socket if necessary
        if (dwSocket != 0)
                closesocket(dwSocket);
                
        dwSocket = 0;

        _endthreadex(0);
        return 0;
}

// called from config.c/UpdateConfig
void SetDWTCPConnectionEnable(unsigned int enable)
{

        // turning us on?
        if ((enable == 1) & (!DWTCPEnabled))
        {
                DWTCPEnabled = true;

                // reset buffer pointers
                InReadPos = 0;
                InWritePos = 0;

                
                // start create thread to handle io
                hDWTCPThread;
                HANDLE hEvent;
                
                unsigned threadID;

                hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ) ;
                
                if (hEvent==NULL)
                {
                      DLOG_C("Cannot create DWTCPConnection thread!\n");
                        return;
                }

                // start it up...
                hDWTCPThread = (HANDLE)_beginthreadex( NULL, 0, &DWTCPThread, hEvent, 0, &threadID );

                if (hDWTCPThread==NULL)
                {
	                    DLOG_C("Cannot start DWTCPConnection thread!\n");
                        return;
                }

                DLOG_C("DWTCPConnection thread started with id %d\n",threadID);
                

        }
        else if ((enable != 1) & DWTCPEnabled)
        {
                // we were running but have been turned off
                DWTCPEnabled = false;
        
                killDWTCPThread();
        }

}

extern "C"
{
	__declspec(dllexport) const char* PakGetName()
	{
		static char string_buffer[MAX_LOADSTRING];

		//LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);
		strcpy(string_buffer, "Bare Becker Port");

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static char string_buffer[MAX_LOADSTRING];

		//LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);
		strcpy(string_buffer, "");

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_DESCRIPTION, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) void PakInitialize(
		slot_id_type SlotId,
		const char* const configuration_path,
		HWND hVccWnd,
		const cpak_callbacks* const callbacks)
	{
		gSlotId = SlotId;
		PakSetCart = callbacks->assert_cartridge_line;
		LastStats = GetTickCount();
        gpSettings = new VCC::Util::settings(configuration_path);
		LoadConfig();
		SetDWTCPConnectionEnable(1);
	}

	__declspec(dllexport) bool PakGetMenuItem(menu_item_entry* item, size_t index)
	{
		return get_menu_item(item, index);
	}

}


extern "C" __declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
	{
		switch (Port)
		{
			// write data 
			case 0x42:
				dw_write(Data);
				break;
		}
		return;
	}


extern "C" __declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
	{
		switch (Port)
		{
			// read status
			case 0x41:
				if (dw_status() != 0)
					return 2;
				else
					return 0;
				break;
			// read data 
			case 0x42:
				return(dw_read());
				break;
		}

		return 0;
	}

/*
	__declspec(dllexport) void PakReset()
	{
		if (PakSetCart!=NULL)
			PakSetCart(1);
		return(0);
	}
*/


extern "C" __declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short Address)
	{
		return(HDBRom[Address & 8191]);
	
	}

extern "C" __declspec(dllexport) void PakProcessHorizontalSync()
	{
		// flush write buffer in the future..?
		return;
	}

extern "C" __declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
	{
        // calculate speed
        DWORD sinceCalc = GetTickCount() - LastStats;
        if (sinceCalc > STATS_PERIOD_MS)
        {
                LastStats += sinceCalc;
                
                ReadSpeed = 8.0f * (BytesReadSince / (1000.0f - sinceCalc));
                WriteSpeed = 8.0f * (BytesWrittenSince / (1000.0f - sinceCalc));

                BytesReadSince = 0;
                BytesWrittenSince = 0;
        }
        

        if (DWTCPEnabled)
        {
                if (retry)
                {
                        sprintf(text_buffer,"DW %s?", curaddress);
                }
                else if (dwSocket == 0)
                {
                        sprintf(text_buffer,"DW ConError");
                }
                else
                {
                        int buffersize = InWritePos - InReadPos;
                        if (InReadPos > InWritePos)
                                buffersize = BUFFER_SIZE - InReadPos + InWritePos;

                        
                        sprintf(text_buffer,"DW OK R:%04.1f W:%04.1f", ReadSpeed , WriteSpeed);

                        
                }
        }
        else
        {
                        sprintf(text_buffer,"");
        }
        return;
	}

bool get_menu_item(menu_item_entry* item, size_t index) {
	if (!item) return false;
	if (index == 0) {
		BeckerMenu.clear();
		BeckerMenu.add("", 0, MIT_Seperator);
		BeckerMenu.add("DriveWire Server..", ControlId(16), MIT_StandAlone);
	}
	return BeckerMenu.copy_item(*item, index);
}

extern "C" __declspec(dllexport) void PakMenuItemClicked(unsigned char MenuID)
	{
		HWND h_own = GetActiveWindow();
		CreateDialog(gModuleInstance,(LPCTSTR)IDD_PROPPAGE,h_own,(DLGPROC)Config);
		ShowWindow(g_hConfigDlg,1);
		return;
	}


LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOwner; 
	RECT rc, rcDlg, rcOwner; 

	switch (message)
	{
    	case WM_CLOSE:
        	DestroyWindow(hDlg);
        	g_hConfigDlg=nullptr;
        	return TRUE;
        	break;

		case WM_INITDIALOG:

			if ((hwndOwner = GetParent(hDlg)) == NULL) 
			{
				hwndOwner = GetDesktopWindow(); 
			}

			g_hConfigDlg=hDlg;

			GetWindowRect(hwndOwner, &rcOwner); 
			GetWindowRect(hDlg, &rcDlg); 
			CopyRect(&rc, &rcOwner); 

			OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
			OffsetRect(&rc, -rc.left, -rc.top); 
			OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

		    SetWindowPos(hDlg, 
                 HWND_TOP, 
                 rcOwner.left + (rc.right / 2), 
                 rcOwner.top + (rc.bottom / 2), 
                 0, 0,          // Ignores size arguments. 
                 SWP_NOSIZE); 

			SendDlgItemMessage (hDlg,IDC_TCPHOST, WM_SETTEXT, 0,(LPARAM)(LPCSTR)dwaddress);
			sprintf(msg,"%d", dwsport);
			SendDlgItemMessage (hDlg,IDC_TCPPORT, WM_SETTEXT, 0,(LPARAM)(LPCSTR)msg);
			
			return TRUE; 
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					
					SendDlgItemMessage (hDlg,IDC_TCPHOST, WM_GETTEXT, MAX_PATH,(LPARAM)(LPCSTR)dwaddress);
					SendDlgItemMessage (hDlg,IDC_TCPPORT, WM_GETTEXT, MAX_PATH,(LPARAM)(LPCSTR)msg);
					dw_setaddr(dwaddress);
					dw_setport(msg);
					SaveConfig();

            		DestroyWindow(hDlg);
            		g_hConfigDlg=nullptr;
					return TRUE;
				break;

				case IDHELP:
					return TRUE;
				break;

				case IDCANCEL:
            		DestroyWindow(hDlg);
            		g_hConfigDlg=nullptr;
				break;
			}
			return TRUE;
		break;

	}
    return FALSE;
}


void LoadConfig()
{
	char ModName[MAX_LOADSTRING]="";
	char saddr[MAX_LOADSTRING]="";
	char sport[MAX_LOADSTRING]="";

	LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);

	Setting().read(ModName,"DWServerAddr","",saddr, MAX_LOADSTRING);
	Setting().read(ModName,"DWServerPort","",sport, MAX_LOADSTRING);
	
	if (strlen(saddr) > 0)
		dw_setaddr(saddr);
	else
		dw_setaddr("127.0.0.1");

	if (strlen(sport) > 0)
		dw_setport(sport);
	else
		dw_setport("65504");
	
	return;
}

void SaveConfig()
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	Setting().write(ModName,"DWServerAddr",dwaddress);
	sprintf(msg, "%d", dwsport);
	Setting().write(ModName,"DWServerPort",msg);
	return;
}

