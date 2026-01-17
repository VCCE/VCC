#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include "becker.h"
#include "resource.h" 
#include "../CartridgeMenu.h"
#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/util/limits.h>
#include <vcc/util/logger.h>
#include <vcc/util/FileOps.h>
#include <vcc/util/DialogOps.h>

// socket
static SOCKET dwSocket = 0;

// vcc stuff
static HINSTANCE gModuleInstance;
static void* gCallbackContext = nullptr;
static PakAppendCartridgeMenuHostCallback CartMenuCallback = nullptr;
static PakAssertCartridgeLineHostCallback PakSetCart = nullptr;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
static char IniFile[MAX_PATH]="";
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

unsigned char LoadExtRom(const char *);
void SetDWTCPConnectionEnable(unsigned int enable);
int dw_setaddr(const char *bufdwaddr);
int dw_setport(const char *bufdwport);
void BuildCartridgeMenu();
void LoadConfig();
void SaveConfig();


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
		strcpy(string_buffer, "HDBDOS/DW/Becker");

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_DESCRIPTION, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) void PakInitialize(
		void* const callback_context,
		const char* const configuration_path,
		HWND hVccWnd,
		const cpak_callbacks* const callbacks)
	{
		gCallbackContext = callback_context;
		CartMenuCallback = callbacks->add_menu_item;
		PakSetCart = callbacks->assert_cartridge_line;
		strcpy(IniFile, configuration_path);

		LastStats = GetTickCount();
		LoadConfig();
		SetDWTCPConnectionEnable(1);
		BuildCartridgeMenu();
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


void BuildCartridgeMenu()
{
	CartMenuCallback(gCallbackContext, "", MID_BEGIN, MIT_Head);
	CartMenuCallback(gCallbackContext, "", MID_ENTRY, MIT_Seperator);
	CartMenuCallback(gCallbackContext, "DriveWire Server..", ControlId(16), MIT_StandAlone);
	CartMenuCallback(gCallbackContext, "", MID_FINISH, MIT_Head);
}

extern "C" __declspec(dllexport) void PakMenuItemClicked(unsigned char MenuID)
	{
		HWND h_own = GetActiveWindow();
		CreateDialog(gModuleInstance,(LPCTSTR)IDD_PROPPAGE,h_own,(DLGPROC)Config);
		ShowWindow(g_hConfigDlg,1);
		BuildCartridgeMenu();
		return;
	}


LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOwner; 
	RECT rc, rcDlg, rcOwner; 

	switch (message)
	{
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

					EndDialog(hDlg, LOWORD(wParam));
					return TRUE;
				break;

				case IDHELP:
					return TRUE;
				break;

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
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
	char DiskRomPath[MAX_PATH];

	LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	GetPrivateProfileString(ModName,"DWServerAddr","",saddr, MAX_LOADSTRING,IniFile);
	GetPrivateProfileString(ModName,"DWServerPort","",sport, MAX_LOADSTRING,IniFile);
	
	if (strlen(saddr) > 0)
		dw_setaddr(saddr);
	else
		dw_setaddr("127.0.0.1");

	if (strlen(sport) > 0)
		dw_setport(sport);
	else
		dw_setport("65504");

	
	BuildCartridgeMenu();
	GetModuleFileName(NULL, DiskRomPath, MAX_PATH);
	PathRemoveFileSpec(DiskRomPath);
	strcat( DiskRomPath, "hdbdwbck.rom");
	LoadExtRom(DiskRomPath);
	return;
}

void SaveConfig()
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(gModuleInstance,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	WritePrivateProfileString(ModName,"DWServerAddr",dwaddress,IniFile);
	sprintf(msg, "%d", dwsport);
	WritePrivateProfileString(ModName,"DWServerPort",msg,IniFile);
	return;
}

unsigned char LoadExtRom( const char *FilePath)	//Returns 1 on if loaded
{

	FILE *rom_handle = NULL;
	unsigned short index = 0;
	unsigned char RetVal = 0;

	rom_handle = fopen(FilePath, "rb");
	if (rom_handle == NULL)
		memset(HDBRom, 0xFF, 8192);
	else
	{
		while ((feof(rom_handle) == 0) & (index<8192))
			HDBRom[index++] = fgetc(rom_handle);
		RetVal = 1;
		fclose(rom_handle);
	}
	return RetVal;
}
