#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <stdio.h>
#include "becker.h"
#include "resource.h" 
#include "../logger.h"
#include "../fileops.h"
#include "../ModuleDefs.h"

#ifndef USE_LOGGING
#define WriteLog(a,b)
#endif

// socket
static SOCKET dwSocket = 0;

// vcc stuff
static HINSTANCE g_hinstDLL=NULL;
static void (*PakSetCart)(unsigned char)=NULL;
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

static DYNAMICMENUCALLBACK DynamicMenuCallback = NULL;
unsigned char LoadExtRom(char *);
void SetDWTCPConnectionEnable(unsigned int enable);
int dw_setaddr(char *bufdwaddr);
int dw_setport(char *bufdwport);
void BuildDynaMenu(void);
void LoadConfig(void);
void SaveConfig(void);


// dll entry hook
BOOL APIENTRY DllMain( HINSTANCE  hinstDLL, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// init
			g_hinstDLL=hinstDLL;
			LastStats = GetTickCount();
			SetDWTCPConnectionEnable(1);

			break;
		case DLL_PROCESS_DETACH:
			// shutdown

		
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
						sprintf(msg,"dw_write: socket error %d\n", WSAGetLastError());
						WriteLog(msg,TOCONS);
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
	              sprintf(msg,"coco write but null socket\n");
	              WriteLog(msg,TOCONS);
	     }

        return 0;
}


void killDWTCPThread(void)
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
int dw_setaddr(char *bufdwaddr)
{
        strcpy(dwaddress,bufdwaddr);
        return 0;
}


// set our port, called from config.c
int dw_setport(char *bufdwport)
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


      sprintf(msg,"Connecting to %s:%d... \n",dwaddress,dwsport);
      WriteLog(msg,TOCONS);

        // resolve hostname
        LPHOSTENT dwSrvHost= gethostbyname(dwaddress);
        
        if (dwSrvHost == NULL)
        {
                // invalid hostname/no dns
                retry = false;
//              WriteLog("failed to resolve hostname.\n",TOCONS);
        }
        
        // allocate socket
        dwSocket = socket (AF_INET,SOCK_STREAM,IPPROTO_TCP);

        if (dwSocket == INVALID_SOCKET)
        {
                // no deal
                retry = false;
              WriteLog("invalid socket.\n", TOCONS);
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
//              WriteLog("failed to connect.\n",TOCONS);
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
                WriteLog("WSAStartup() failed, DWTCPConnection thread exiting\n",TOCONS);
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

               // WriteLog("DWTCPConnection has been enabled.\n",TOCONS);

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
                      WriteLog("Cannot create DWTCPConnection thread!\n",TOCONS);
                        return;
                }

                // start it up...
                hDWTCPThread = (HANDLE)_beginthreadex( NULL, 0, &DWTCPThread, hEvent, 0, &threadID );

                if (hDWTCPThread==NULL)
                {
	                    WriteLog("Cannot start DWTCPConnection thread!\n",TOCONS);
                        return;
                }

                sprintf(msg,"DWTCPConnection thread started with id %d\n",threadID);
                WriteLog(msg,TOCONS);
                

        }
        else if ((enable != 1) & DWTCPEnabled)
        {
                // we were running but have been turned off
                DWTCPEnabled = false;
        
                killDWTCPThread();
        
                // WriteLog("DWTCPConnection has been disabled.\n",TOCONS);
        
        }

}









// dll exported functions
extern "C" __declspec(dllexport) void ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
	{
		LoadString(g_hinstDLL,IDS_MODULE_NAME, ModName, MAX_LOADSTRING);
		LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);		
		strcpy(ModName,"HDBDOS/DW/Becker");

		DynamicMenuCallback =Temp;
		if (DynamicMenuCallback  != NULL)
			BuildDynaMenu();

		return ;
	}

extern "C" __declspec(dllexport) void PackPortWrite(unsigned char Port,unsigned char Data)
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


extern "C" __declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
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
	__declspec(dllexport) unsigned char ModuleReset(void)
	{
		if (PakSetCart!=NULL)
			PakSetCart(1);
		return(0);
	}
*/
extern "C" __declspec(dllexport) unsigned char SetCart(SETCART Pointer)
	{
		
		PakSetCart=Pointer;
		return 0;
	}

extern "C" __declspec(dllexport) unsigned char PakMemRead8(unsigned short Address)
	{
		//sprintf(msg,"PalMemRead8: addr %d  val %d\n",(Address & 8191), Rom[Address & 8191]);
        //WriteLog(msg,TOCONS);
		return(HDBRom[Address & 8191]);
	
	}

extern "C" __declspec(dllexport) void HeartBeat(void)
	{
		// flush write buffer in the future..?
		return;
	}

extern "C" __declspec(dllexport) void ModuleStatus(char *DWStatus)
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
                        sprintf(DWStatus,"DW %s?", curaddress);
                }
                else if (dwSocket == 0)
                {
                        sprintf(DWStatus,"DW ConError");
                }
                else
                {
                        int buffersize = InWritePos - InReadPos;
                        if (InReadPos > InWritePos)
                                buffersize = BUFFER_SIZE - InReadPos + InWritePos;

                        
                        sprintf(DWStatus,"DW OK R:%04.1f W:%04.1f", ReadSpeed , WriteSpeed);

                        
                }
        }
        else
        {
                        sprintf(DWStatus,"");
        }
        return;
	}


void BuildDynaMenu(void)
{
	DynamicMenuCallback( "",MID_BEGIN,MIT_Head);
	DynamicMenuCallback( "",MID_ENTRY,MIT_Seperator);
	DynamicMenuCallback( "DriveWire Server..",5016,MIT_StandAlone);
	DynamicMenuCallback( "",MID_FINISH,MIT_Head);
}

extern "C" __declspec(dllexport) void ModuleConfig(unsigned char MenuID)
	{
		HWND h_own = GetActiveWindow();
		CreateDialog(g_hinstDLL,(LPCTSTR)IDD_PROPPAGE,h_own,(DLGPROC)Config);
		ShowWindow(g_hConfigDlg,1);
		BuildDynaMenu();
		return;
	}

extern "C" __declspec(dllexport) void SetIniPath (char *IniFilePath)
	{
		strcpy(IniFile,IniFilePath);
		LoadConfig();
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



void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	char saddr[MAX_LOADSTRING]="";
	char sport[MAX_LOADSTRING]="";
	char DiskRomPath[MAX_PATH];

	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
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

	
	BuildDynaMenu();
	GetModuleFileName(NULL, DiskRomPath, MAX_PATH);
	PathRemoveFileSpec(DiskRomPath);
	strcat( DiskRomPath, "hdbdwbck.rom");
	LoadExtRom(DiskRomPath);
	return;
}

void SaveConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	WritePrivateProfileString(ModName,"DWServerAddr",dwaddress,IniFile);
	sprintf(msg, "%d", dwsport);
	WritePrivateProfileString(ModName,"DWServerPort",msg,IniFile);
	return;
}

unsigned char LoadExtRom( char *FilePath)	//Returns 1 on if loaded
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
