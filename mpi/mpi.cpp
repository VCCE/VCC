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
*/
/****************************************************************************/
/* 
	This is an expansion module for the Vcc Emulator. 
	
	It simulated the functions of the TRS-80 Multi-Pak Interface

	TODO: <snippet from the Cloud9 web site>

		"Here's a simple test that will determine if your Mulit-Pak has already 
		been upgraded to work with a CoCo 3. From BASIC's OK prompt, 
		type PRINT PEEK(&HFF90) and press ENTER. 
		If 126 is returned then the Multi-Pak has been upgraded. 
		If it returns 255 then it has NOT been upgraded."

		VCC is currently producing 204 as the return value

	TODO: VCC GetStatus shoud be changed to use safe versions of sprintf, strcat
			and so should each Pak (including us)
*/
/****************************************************************************/

#include "mpi.h"

#include "../fileops.h"
#include "../vccPakAPI.h"

#include <stdio.h>
#include <assert.h>

#include <windows.h>
#include <commctrl.h>
#include "resource.h" 

/****************************************************************************/
/*
	Forward declarations
*/

void SetCartSlot0(unsigned char);
void SetCartSlot1(unsigned char);
void SetCartSlot2(unsigned char);
void SetCartSlot3(unsigned char);
void BuildDynaMenu(void);
void DynamicMenuCallback0(char *, int, int);
void DynamicMenuCallback1(char *, int, int);
void DynamicMenuCallback2(char *, int, int);
void DynamicMenuCallback3(char *, int, int);

/****************************************************************************/
/*
	Local definitions
*/

#define MAXPAX			(1<<2)	///< Maximum number of Paks, needs to be a power of 2, not more than (1<<4)
#define MAX_MENUS		64		///< Maximum menus per Pak
#define MAX_MENU_SIZE	512		///< Maximum text size for a menu item

/****************************************************************************/

// globsal/common functions provided by VCC that will be 
// called by the Pak to interface with the emulator
static vccpakapi_assertinterrupt_t	AssertInt			= NULL;
static vcccpu_read8_t				MemRead8			= NULL;
static vcccpu_write8_t				MemWrite8			= NULL;
static vccapi_setcart_t				PakSetCart			= NULL;
static vccapi_dynamicmenucallback_t	DynamicMenuCallback = NULL;

//
// Arrays of fuction pointers, etc, for each Slot
//
// TODO: change to an array of slots, each slot contains vccpakapi_t, etc once it is defined
//       in VCC and the Pak API is expanded.  when the VCC core is put into a library
//       some of these API calls to get function pointers from VCC can be removed
//
//static INITIALIZE				InitializeCalls[MAXPAX]				= { NULL,NULL,NULL,NULL };
static vccpakapi_getname_t				GetModuleNameCalls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static vccpakapi_config_t				ConfigModuleCalls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static vccpakapi_heartbeat_t			HeartBeatCalls[MAXPAX]				= {NULL,NULL,NULL,NULL};
static vccpakapi_portwrite_t			PakPortWriteCalls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static vccpakapi_portread_t				PakPortReadCalls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static vcccpu_write8_t					PakMemWrite8Calls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static vcccpu_read8_t					PakMemRead8Calls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static vccpakapi_status_t				ModuleStatusCalls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static vccpakapi_getaudiosample_t		ModuleAudioSampleCalls[MAXPAX]		= {NULL,NULL,NULL,NULL};
static vccpakapi_reset_t				ModuleResetCalls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static vccapi_setcart_t					SetCarts[MAXPAX]					= { SetCartSlot0,SetCartSlot1,SetCartSlot2,SetCartSlot3 };
static vccapi_dynamicmenucallback_t		DynamicMenuCallbackCalls[MAXPAX]	= { DynamicMenuCallback0,DynamicMenuCallback1,DynamicMenuCallback2,DynamicMenuCallback3 };
static vccpakapi_setcartptr_t			SetCartCalls[MAXPAX]				= { NULL,NULL,NULL,NULL };
static vccpakapi_setinipath_t			SetIniPathCalls[MAXPAX]				= { NULL,NULL,NULL,NULL };
// Set callbacks for the DLL to call
static vccpakapi_setintptr_t			SetInteruptCallPointerCalls[MAXPAX]	= {NULL,NULL,NULL,NULL};
static vccpakapi_setmemptrs_t			DmaMemPointerCalls[MAXPAX]			= {NULL,NULL,NULL,NULL};
static HINSTANCE						hinstLib[MAXPAX]					= { NULL,NULL,NULL,NULL };
static char								ModuleNames[MAXPAX][MAX_LOADSTRING] = { "Empty","Empty","Empty","Empty" };
static char								CatNumber[MAXPAX][MAX_LOADSTRING]	= { "","","","" };
static char								SlotLabel[MAXPAX][MAX_LOADSTRING * 2] = { "Empty","Empty","Empty","Empty" };
static char								ModulePaths[MAXPAX][MAX_PATH]		= { "","","","" };
static unsigned char *					ExtRomPointers[MAXPAX]				= { NULL,NULL,NULL,NULL };
static unsigned int						BankedCartOffset[MAXPAX]			= { 0,0,0,0 };

static char MenuName0[MAX_MENUS][MAX_MENU_SIZE];
static char MenuName1[MAX_MENUS][MAX_MENU_SIZE];
static char MenuName2[MAX_MENUS][MAX_MENU_SIZE];
static char MenuName3[MAX_MENUS][MAX_MENU_SIZE];
static int MenuId0[MAX_MENUS];
static int MenuId1[MAX_MENUS];
static int MenuId2[MAX_MENUS];
static int MenuId3[MAX_MENUS];
static int Type0[MAX_MENUS];
static int Type1[MAX_MENUS];
static int Type2[MAX_MENUS];
static int Type3[MAX_MENUS];
static int MenuIndex[MAXPAX]={0,0,0,0};

// CART enabled per slot
static unsigned char CartForSlot[MAXPAX]={0,0,0,0};
// MPI variables
static unsigned char ChipSelectSlot = 3;
static unsigned char SpareSelectSlot = 3;
static unsigned char SwitchSlot = 3;
static unsigned char SlotRegister = 255;
static HINSTANCE g_hinstDLL = NULL;
static HWND g_hWnd = NULL;
static char IniFile[MAX_PATH] = "";
static char LastPath[MAX_PATH] = "";

/****************************************************************************/
/**
*/
int FileID(char *Filename)
{
	FILE *DummyHandle = NULL;
	char Temp[3] = "";
	DummyHandle = fopen(Filename, "rb");
	if (DummyHandle == NULL)
		return(0);	//File Doesn't exist

	Temp[0] = fgetc(DummyHandle);
	Temp[1] = fgetc(DummyHandle);
	Temp[2] = 0;
	fclose(DummyHandle);
	if (strcmp(Temp, "MZ") == 0)
		return(1);	//DLL File
	return(2);		//Rom Image 
}

/**
*/
void UnloadModule(unsigned char Slot)
{
	GetModuleNameCalls[Slot] = NULL;
	ConfigModuleCalls[Slot] = NULL;
	PakPortWriteCalls[Slot] = NULL;
	PakPortReadCalls[Slot] = NULL;
	SetInteruptCallPointerCalls[Slot] = NULL;
	DmaMemPointerCalls[Slot] = NULL;
	HeartBeatCalls[Slot] = NULL;
	PakMemWrite8Calls[Slot] = NULL;
	PakMemRead8Calls[Slot] = NULL;
	ModuleStatusCalls[Slot] = NULL;
	ModuleAudioSampleCalls[Slot] = NULL;
	ModuleResetCalls[Slot] = NULL;
	SetIniPathCalls[Slot] = NULL;
	//	SetCartCalls[Slot]=NULL;
	strcpy(ModulePaths[Slot], "");
	strcpy(ModuleNames[Slot], "Empty");
	strcpy(CatNumber[Slot], "");
	strcpy(SlotLabel[Slot], "Empty");
	if (hinstLib[Slot] != NULL)
		FreeLibrary(hinstLib[Slot]);
	if (ExtRomPointers[Slot] != NULL)
		free(ExtRomPointers[Slot]);
	hinstLib[Slot] = NULL;
	ExtRomPointers[Slot] = NULL;
	CartForSlot[Slot] = 0;
	MenuIndex[Slot] = 0;
	return;
}

/**
*/
unsigned char MountModule(unsigned char Slot,char *ModName)
{
	unsigned char ModuleType=0;
	char ModuleName[MAX_PATH]="";
	unsigned int index=0;
	strcpy(ModuleName,ModName);
	FILE *rom_handle;
	if (Slot > MAXPAX - 1)
	{
		return(0);
	}
	ModuleType=FileID(ModuleName);
	switch (ModuleType)
	{
	case 0: //File doesn't exist
		return(0);
	break;

	case 2: //ROM image
		UnloadModule(Slot);
		ExtRomPointers[Slot]=(unsigned char *)malloc(0x40000);
		if (ExtRomPointers[Slot]==NULL)
		{
			MessageBox(0,"Rom pointer is NULL","Error",0);
			return(0); //Can Allocate RAM
		}
		rom_handle=fopen(ModuleName,"rb");
		if (rom_handle==NULL)
		{
			MessageBox(0,"File handle is NULL","Error",0);
			return(0);
		}
		while ((feof(rom_handle)==0) & (index<0x40000))
			ExtRomPointers[Slot][index++]=fgetc(rom_handle);
		fclose(rom_handle);
		strcpy(ModulePaths[Slot],ModuleName);
		PathStripPath(ModuleName);
//		PathRemovePath(ModuleName);
		PathRemoveExtension(ModuleName);
		strcpy(ModuleNames[Slot],ModuleName);
		strcpy(SlotLabel[Slot],ModuleName); //JF
		CartForSlot[Slot]=1;
//		if (CartForSlot[SpareSelectSlot]==1)
//			PakSetCart(1);
		return(1);
	break;

	case 1:	//DLL File
		UnloadModule(Slot);
		strcpy(ModulePaths[Slot],ModuleName);
		hinstLib[Slot] = LoadLibrary(ModuleName);
		if (hinstLib[Slot] ==NULL)
			return(0);	//Error Can't open File

		GetModuleNameCalls[Slot]=(vccpakapi_getname_t)GetProcAddress(hinstLib[Slot], VCC_PAKAPI_GETNAME);
		ConfigModuleCalls[Slot]=(vccpakapi_config_t)GetProcAddress(hinstLib[Slot], VCC_PAKAPI_CONFIG);
		PakPortWriteCalls[Slot]=(vccpakapi_portwrite_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_PORTWRITE);
		PakPortReadCalls[Slot]=(vccpakapi_portread_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_PORTREAD);
		SetInteruptCallPointerCalls[Slot]=(vccpakapi_setintptr_t)GetProcAddress(hinstLib[Slot], VCC_PAKAPI_ASSERTINTERRUPT);

		DmaMemPointerCalls[Slot]=(vccpakapi_setmemptrs_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_MEMPOINTERS);
		SetCartCalls[Slot]=(vccpakapi_setcartptr_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_SETCART);
		
		HeartBeatCalls[Slot]=(vccpakapi_heartbeat_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_HEARTBEAT);
		PakMemWrite8Calls[Slot]=(vcccpu_write8_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_MEMWRITE);
		PakMemRead8Calls[Slot]=(vcccpu_read8_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_MEMREAD);
		ModuleStatusCalls[Slot]=(vccpakapi_status_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_STATUS);
		ModuleAudioSampleCalls[Slot]=(vccpakapi_getaudiosample_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_AUDIOSAMPLE);
		ModuleResetCalls[Slot]=(vccpakapi_reset_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_RESET);
		SetIniPathCalls[Slot]=(vccpakapi_setinipath_t) GetProcAddress(hinstLib[Slot], VCC_PAKAPI_SETINIPATH);

		if (GetModuleNameCalls[Slot] == NULL)
		{
			UnloadModule(Slot);
			MessageBox(0,"Not a valid Module","Ok",0);
			return(0); //Error Not a Vcc Module 
		}
		// TODO: Need to add address of local Dynamic menu callback function!
		GetModuleNameCalls[Slot](ModuleNames[Slot],CatNumber[Slot],DynamicMenuCallbackCalls[Slot],g_hWnd);
		strcpy(SlotLabel[Slot],ModuleNames[Slot]);
		strcat(SlotLabel[Slot],"  ");
		strcat(SlotLabel[Slot],CatNumber[Slot]);

		if (SetInteruptCallPointerCalls[Slot] !=NULL)
			SetInteruptCallPointerCalls[Slot](AssertInt);
		if (DmaMemPointerCalls[Slot] !=NULL)
			DmaMemPointerCalls[Slot](MemRead8,MemWrite8);
		if (SetIniPathCalls[Slot] != NULL)
			SetIniPathCalls[Slot](IniFile);
		if (SetCartCalls[Slot] !=NULL)
			SetCartCalls[Slot](*SetCarts[Slot]);	//Transfer the address of the SetCart routin to the pak
													//For the multpak there is 1 for each slot se we know where it came from
		if (ModuleResetCalls[Slot]!=NULL)
			ModuleResetCalls[Slot]();
//		if (CartForSlot[SpareSelectSlot]==1)
//			PakSetCart(1);
		return(1);
	break;
	}
	return(0);
}

/**
*/
void LoadCartDLL(unsigned char Slot,char *DllPath)
{
	OPENFILENAME ofn ;
	unsigned char RetVal=0;

	UnloadModule(Slot);
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = g_hWnd;
	ofn.lpstrFilter       =	"Program Packs\0*.ROM;*.DLL\0\0" ;			// filter string
	ofn.nFilterIndex      = 1 ;								// current filter index
	ofn.lpstrFile         = DllPath;						// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;							// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
	ofn.lpstrTitle        = TEXT("Load Program Pack");	// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	ofn.lpstrInitialDir = NULL;							// initial directory
	if (strlen(LastPath) > 0)
	{
		ofn.lpstrInitialDir = LastPath;
	}

	if ( GetOpenFileName (&ofn) )
	{
		// save last path
		strcpy(LastPath, DllPath);
		PathRemoveFileSpec(LastPath);

		RetVal = MountModule( Slot,DllPath);
	}
}

/**
*/
void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	SwitchSlot=GetPrivateProfileInt(ModName,"SWPOSITION",3,IniFile);
	ChipSelectSlot=SwitchSlot;
	SpareSelectSlot=SwitchSlot;
	GetPrivateProfileString(ModName,"SLOT1","",ModulePaths[0],MAX_PATH,IniFile);
	CheckPath(ModulePaths[0]);
	GetPrivateProfileString(ModName,"SLOT2","",ModulePaths[1],MAX_PATH,IniFile);
	CheckPath(ModulePaths[1]);
	GetPrivateProfileString(ModName,"SLOT3","",ModulePaths[2],MAX_PATH,IniFile);
	CheckPath(ModulePaths[2]);
	GetPrivateProfileString(ModName,"SLOT4","",ModulePaths[3],MAX_PATH,IniFile);
	CheckPath(ModulePaths[3]);
	for (int Temp = 0; Temp < MAXPAX; Temp++)
	{
		if (strlen(ModulePaths[Temp]) != 0)
		{
			MountModule(Temp, ModulePaths[Temp]);
		}
	}

	GetPrivateProfileString(ModName, "LastPath", "", LastPath, MAX_PATH, IniFile);

	BuildDynaMenu();
}

/**
*/
void WriteConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	WritePrivateProfileInt(ModName,"SWPOSITION",SwitchSlot,IniFile);
	ValidatePath(ModulePaths[0]);
	WritePrivateProfileString(ModName,"SLOT1",ModulePaths[0],IniFile);
	ValidatePath(ModulePaths[1]);
	WritePrivateProfileString(ModName,"SLOT2",ModulePaths[1],IniFile);
	ValidatePath(ModulePaths[2]);
	WritePrivateProfileString(ModName,"SLOT3",ModulePaths[2],IniFile);
	ValidatePath(ModulePaths[3]);
	WritePrivateProfileString(ModName,"SLOT4",ModulePaths[3],IniFile);
	
	WritePrivateProfileString(ModName, "LastPath", LastPath, IniFile);
}

/**
*/
void ReadModuleParms(unsigned char Slot,char *String)
{
	strcat(String,"Module Name: ");
	strcat(String,ModuleNames[Slot]);

	strcat(String,"\r\n-----------------------------------------\r\n");

	if (ConfigModuleCalls[Slot]!=NULL)
		strcat(String,"Has Configurable options\r\n");

	if (SetIniPathCalls[Slot]!=NULL)
		strcat(String,"Saves Config Info\r\n");

	if (PakPortWriteCalls[Slot]!=NULL)
		strcat(String,"Is IO writable\r\n");

	if (PakPortReadCalls[Slot]!=NULL)
		strcat(String,"Is IO readable\r\n");

	if (SetInteruptCallPointerCalls[Slot]!=NULL)
		strcat(String,"Generates Interupts\r\n");

	if (DmaMemPointerCalls[Slot]!=NULL)
		strcat(String,"Generates DMA Requests\r\n");

	if (HeartBeatCalls[Slot]!=NULL)
		strcat(String,"Needs Heartbeat\r\n");

	if (ModuleAudioSampleCalls[Slot]!=NULL)
		strcat(String,"Analog Audio Outputs\r\n");

	if (PakMemWrite8Calls[Slot]!=NULL)
		strcat(String,"Needs CS Write\r\n");

	if (PakMemRead8Calls[Slot]!=NULL)
		strcat(String,"Needs CS Read (onboard ROM)\r\n");

	if (ModuleStatusCalls[Slot]!=NULL)
		strcat(String,"Returns Status\r\n");

	if (ModuleResetCalls[Slot]!=NULL)
		strcat(String,"Needs Reset Notification\r\n");

//	if (SetCartCalls[Slot]!=NULL)
//		strcat(String,"Asserts CART\r\n");
}

void SetCartSlot0(unsigned char Tmp)
{
	CartForSlot[0]=Tmp;
	return;
}

void SetCartSlot1(unsigned char Tmp)
{
	CartForSlot[1]=Tmp;
	return;
}

void SetCartSlot2(unsigned char Tmp)
{
	CartForSlot[2]=Tmp;
	return;
}

void SetCartSlot3(unsigned char Tmp)
{
	CartForSlot[3]=Tmp;
	return;
}

void BuildDynaMenu(void)	//STUB
{
	unsigned char TempIndex=0;
	char TempMsg[512]="";
	if (DynamicMenuCallback ==NULL)
		MessageBox(0,"No good","Ok",0);
	DynamicMenuCallback( "",0,0);
	DynamicMenuCallback( "",6000,0);
	DynamicMenuCallback( "MPI Slot 4",6000,HEAD);
	DynamicMenuCallback( "Insert",5010,SLAVE);
	sprintf(TempMsg,"Eject: ");
	strcat(TempMsg,SlotLabel[3]);
	DynamicMenuCallback( TempMsg,5011,SLAVE);
	DynamicMenuCallback( "MPI Slot 3",6000,HEAD);
	DynamicMenuCallback( "Insert",5012,SLAVE);
	sprintf(TempMsg,"Eject: ");
	strcat(TempMsg,SlotLabel[2]);
	DynamicMenuCallback( TempMsg,5013,SLAVE);
	DynamicMenuCallback( "MPI Slot 2",6000,HEAD);
	DynamicMenuCallback( "Insert",5014,SLAVE);
	sprintf(TempMsg,"Eject: ");
	strcat(TempMsg,SlotLabel[1]);
	DynamicMenuCallback( TempMsg,5015,SLAVE);
	DynamicMenuCallback( "MPI Slot 1",6000,HEAD);
	DynamicMenuCallback( "Insert",5016,SLAVE);
	sprintf(TempMsg,"Eject: ");
	strcat(TempMsg,SlotLabel[0]);
	DynamicMenuCallback( TempMsg,5017,SLAVE);
	DynamicMenuCallback( "MPI Config",5018,STANDALONE);

	for (TempIndex=0;TempIndex<MenuIndex[3];TempIndex++)
		DynamicMenuCallback(MenuName3[TempIndex],MenuId3[TempIndex]+80,Type3[TempIndex]);

	for (TempIndex=0;TempIndex<MenuIndex[2];TempIndex++)
		DynamicMenuCallback(MenuName2[TempIndex],MenuId2[TempIndex]+60,Type2[TempIndex]);

	for (TempIndex=0;TempIndex<MenuIndex[1];TempIndex++)
		DynamicMenuCallback(MenuName1[TempIndex],MenuId1[TempIndex]+40,Type1[TempIndex]);

	for (TempIndex=0;TempIndex<MenuIndex[0];TempIndex++)
		DynamicMenuCallback(MenuName0[TempIndex],MenuId0[TempIndex]+20,Type0[TempIndex]);

	DynamicMenuCallback( "",1,0);
}

void DynamicMenuCallback0( char *MenuName,int MenuId, int Type)
{
	if (MenuId==0)
	{
		MenuIndex[0]=0;
		return;
	}

	if (MenuId==1)
	{
		BuildDynaMenu();
		return;
	}

	strcpy(MenuName0[MenuIndex[0]],MenuName);
	MenuId0[MenuIndex[0]]=MenuId;
	Type0[MenuIndex[0]]=Type;
	MenuIndex[0]++;
}

void DynamicMenuCallback1( char *MenuName,int MenuId, int Type)
{
	if (MenuId==0)
	{
		MenuIndex[1]=0;
		return;
	}

	if (MenuId==1)
	{
		BuildDynaMenu();
		return;
	}
	strcpy(MenuName1[MenuIndex[1]],MenuName);
	MenuId1[MenuIndex[1]]=MenuId;
	Type1[MenuIndex[1]]=Type;
	MenuIndex[1]++;
}

void DynamicMenuCallback2( char *MenuName,int MenuId, int Type)
{
	if (MenuId==0)
	{
		MenuIndex[2]=0;
		return;
	}

	if (MenuId==1)
	{
		BuildDynaMenu();
		return;
	}
	strcpy(MenuName2[MenuIndex[2]],MenuName);
	MenuId2[MenuIndex[2]]=MenuId;
	Type2[MenuIndex[2]]=Type;
	MenuIndex[2]++;
}

void DynamicMenuCallback3( char *MenuName,int MenuId, int Type)
{
	if (MenuId==0)
	{
		MenuIndex[3]=0;
		return;
	}

	if (MenuId==1)
	{
		BuildDynaMenu();
		return;
	}
	strcpy(MenuName3[MenuIndex[3]],MenuName);
	MenuId3[MenuIndex[3]]=MenuId;
	Type3[MenuIndex[3]]=Type;
	MenuIndex[3]++;
}

/****************************************************************************/
/**
	(Windows) Config dialog Window Procedure function
*/
LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned short EDITBOXS[4] = { IDC_EDIT1,IDC_EDIT2,IDC_EDIT3,IDC_EDIT4 };
	unsigned short INSERTBTN[4] = { ID_INSERT1,ID_INSERT2,ID_INSERT3,ID_INSERT4 };
	unsigned short REMOVEBTN[4] = { ID_REMOVE1,ID_REMOVE2,ID_REMOVE3,ID_REMOVE4 };
	unsigned short CONFIGBTN[4] = { ID_CONFIG1,ID_CONFIG2,ID_CONFIG3,ID_CONFIG4 };
	char ConfigText[1024] = "";

	unsigned char Temp = 0;
	switch (message)
	{
	case WM_INITDIALOG:
		for (Temp = 0; Temp < MAXPAX; Temp++)
		{
			SendDlgItemMessage(hDlg, EDITBOXS[Temp], WM_SETTEXT, 5, (LPARAM)(LPCSTR)SlotLabel[Temp]);
		}
		SendDlgItemMessage(hDlg, IDC_PAKSELECT, TBM_SETRANGE, TRUE, MAKELONG(0, 3));
		SendDlgItemMessage(hDlg, IDC_PAKSELECT, TBM_SETPOS, TRUE, SwitchSlot);
		ReadModuleParms(SwitchSlot, ConfigText);
		SendDlgItemMessage(hDlg, IDC_MODINFO, WM_SETTEXT, strlen(ConfigText), (LPARAM)(LPCSTR)ConfigText);

		return TRUE;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			WriteConfig();
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;

		} //End switch LOWORD

		for (Temp = 0; Temp<MAXPAX; Temp++)
		{
			if (LOWORD(wParam) == INSERTBTN[Temp])
			{
				LoadCartDLL(Temp, ModulePaths[Temp]);
				for (Temp = 0; Temp < MAXPAX; Temp++)
				{
					SendDlgItemMessage(hDlg, EDITBOXS[Temp], WM_SETTEXT, strlen(SlotLabel[Temp]), (LPARAM)(LPCSTR)SlotLabel[Temp]);
				}
			}
		}

		for (Temp = 0; Temp<MAXPAX; Temp++)
		{
			if (LOWORD(wParam) == REMOVEBTN[Temp])
			{
				UnloadModule(Temp);
				SendDlgItemMessage(hDlg, EDITBOXS[Temp], WM_SETTEXT, strlen(SlotLabel[Temp]), (LPARAM)(LPCSTR)SlotLabel[Temp]);
			}
		}

		for (Temp = 0; Temp<MAXPAX; Temp++)
		{
			if (LOWORD(wParam) == CONFIGBTN[Temp])
			{
				if (ConfigModuleCalls[Temp] != NULL)
					ConfigModuleCalls[Temp](NULL);
			}
		}
		return TRUE;
		break;

	case WM_HSCROLL:
		SwitchSlot = (unsigned char)SendDlgItemMessage(hDlg, IDC_PAKSELECT, TBM_GETPOS, (WPARAM)0, (WPARAM)0);
		SpareSelectSlot = SwitchSlot;
		ChipSelectSlot = SwitchSlot;
		ReadModuleParms(SwitchSlot, ConfigText);
		SendDlgItemMessage(hDlg, IDC_MODINFO, WM_SETTEXT, strlen(ConfigText), (LPARAM)(LPCSTR)ConfigText);
		PakSetCart(0);
		if (CartForSlot[SpareSelectSlot] == 1)
		{
			PakSetCart(1);
		}
		break;
	}
	return FALSE;
}

/****************************************************************************/
/****************************************************************************/
/*
	VCC Pak API
*/
/****************************************************************************/

/**
	Return our module name to the emulator
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_GETNAME(char * ModName, char * CatNumber, vccapi_dynamicmenucallback_t Temp, void * wndHandle)
{
	g_hWnd = (HWND)wndHandle;

	LoadString(g_hinstDLL, IDS_MODULE_NAME, ModName, MAX_LOADSTRING);
	LoadString(g_hinstDLL, IDS_CATNUMBER, CatNumber, MAX_LOADSTRING);

	DynamicMenuCallback = Temp;
}

/**
	Call config UI
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_CONFIG(unsigned char MenuID)
{
	if ((MenuID >= 10) & (MenuID <= 19)) //Local config menus
		switch (MenuID)
		{
		case 10:
			LoadCartDLL(3, ModulePaths[3]);
			WriteConfig();
			BuildDynaMenu();
			break;

		case 11:
			UnloadModule(3);
			BuildDynaMenu();
			break;
		case 12:
			LoadCartDLL(2, ModulePaths[2]);
			WriteConfig();
			BuildDynaMenu();
			break;
		case 13:
			UnloadModule(2);
			BuildDynaMenu();
			break;
		case 14:
			LoadCartDLL(1, ModulePaths[1]);
			WriteConfig();
			BuildDynaMenu();
			break;
		case 15:
			UnloadModule(1);
			BuildDynaMenu();
			break;
		case 16:
			LoadCartDLL(0, ModulePaths[0]);
			WriteConfig();
			BuildDynaMenu();
			break;
		case 17:
			UnloadModule(0);
			BuildDynaMenu();
			break;
		case 18:
			DialogBox(g_hinstDLL, (LPCTSTR)IDD_DIALOG1, NULL, (DLGPROC)Config);
			break;
		case 19:

			break;
		}

	//Add calls to sub-modules here		
	if ((MenuID >= 20) & (MenuID <= 40))
		ConfigModuleCalls[0](MenuID - 20);

	if ((MenuID>40) & (MenuID <= 60))
		ConfigModuleCalls[1](MenuID - 40);

	if ((MenuID>61) & (MenuID <= 80))
		ConfigModuleCalls[2](MenuID - 60);

	if ((MenuID>80) & (MenuID <= 100))
		ConfigModuleCalls[3](MenuID - 80);
}

/**
	This captures the Function transfer point for the CPU assert interupt
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_ASSERTINTERRUPT(vccpakapi_assertinterrupt_t Dummy)
{
	AssertInt = Dummy;

	for (int Temp = 0; Temp<MAXPAX; Temp++)
	{
		if (SetInteruptCallPointerCalls[Temp] != NULL)
		{
			SetInteruptCallPointerCalls[Temp](AssertInt);
		}
	}
}

/**
	Write a byte to one of our i/o ports
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_PORTWRITE(unsigned char Port, unsigned char Data)
{
	if (Port == 0x7F) //Addressing the Multi-Pak
	{
		SpareSelectSlot = (Data & (MAXPAX-1));
		ChipSelectSlot  = ((Data >> 4) & (MAXPAX-1));
		assert(SpareSelectSlot < MAXPAX && ChipSelectSlot < MAXPAX);
		SlotRegister = Data;

		PakSetCart(0);
		if (CartForSlot[SpareSelectSlot] == 1)
		{
			PakSetCart(1);
		}
	}
	else
	if ( (Port>=0x40) & (Port<0x7F) )
	{
		//BankedCartOffset[SpareSelectSlot] = (Data & 15)<<14;

		if (PakPortWriteCalls[SpareSelectSlot] != NULL)
		{
			PakPortWriteCalls[SpareSelectSlot](Port, Data);
		}
	}
	else
	{
		// Find a Module that we can write to
		// TODO: what does the real MPI do here?
		// Should we even ever get to this point?
		// this assumes there is no conflict between two carts
		// and writes to the lowest numbered slot
		// that will accept it
		for (int Temp = 0; Temp < MAXPAX; Temp++)
		{
			if (PakPortWriteCalls[Temp] != NULL)
			{
				PakPortWriteCalls[Temp](Port, Data);
			}
		}
	}
}

/**
	Read a byte from one of our i/o ports
*/
extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_PORTREAD(unsigned char Port)
{
	// MPI register?
	if ( Port == 0x7F )
	{
		assert(SpareSelectSlot < MAXPAX && ChipSelectSlot < MAXPAX);
		SlotRegister |= (SpareSelectSlot | (ChipSelectSlot << 4));

		return(SlotRegister);
	}
	else
	// SCS range?
	if ( (Port>=0x40) & (Port<0x7F) )
	{
		if ( PakPortReadCalls[SpareSelectSlot] != NULL )
		{
			return PakPortReadCalls[SpareSelectSlot](Port);
		}
		
		// is this what the MPI would return
		// if there is no CART in that slot
		return 0;
	}
	else
	{
		// Find a Module that returns a value
		// TODO: what does the real MPI do here?
		// Should we even ever get to this point?
		// this assumes there is no conflict between two carts
		// and takes the first 'valid' value starting
		// from the lowest numbered slot
		for (int Temp = 0; Temp < MAXPAX; Temp++)
		{
			if ( PakPortReadCalls[Temp] != NULL )
			{
				int Temp2 = PakPortReadCalls[Temp](Port);
				if (Temp2 != 0)
				{
					return(Temp2);
				}
			}
		}
	}

	return(0);
}

/**
	Periodic call from the emulator to us if we or our Paks need it
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_HEARTBEAT(void)
{
	for (int Temp = 0; Temp <MAXPAX; Temp++)
	{
		if (HeartBeatCalls[Temp] != NULL)
		{
			HeartBeatCalls[Temp]();
		}
	}
}


/**
	This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_MEMPOINTERS(vcccpu_read8_t Temp1, vcccpu_write8_t Temp2)
{
	MemRead8 = Temp1;
	MemWrite8 = Temp2;
}

/**
	Read byte from memory (ROM) in the current CTS slot
*/
extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_MEMREAD(unsigned short Address)
{
	if (ExtRomPointers[ChipSelectSlot] != NULL)
	{
		return(ExtRomPointers[ChipSelectSlot][(Address & 32767) + BankedCartOffset[ChipSelectSlot]]); //Bank Select ???
	}

	if (PakMemRead8Calls[ChipSelectSlot] != NULL)
	{
		return(PakMemRead8Calls[ChipSelectSlot](Address));
	}

	return 0;
}

/**
	Memory write (not implemented - cannot write to general addresses for us
	or connected Paks)
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_MEMWRITE(unsigned char Data, unsigned short Address)
{

}

/**
	Get module status

	Return our status plus the status of all Paks appended
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_STATUS(char *MyStatus)
{
	char TempStatus[256] = "";

	sprintf(MyStatus, "MPI:%i,%i", ChipSelectSlot, SpareSelectSlot);
	for (int Temp = 0; Temp<MAXPAX; Temp++)
	{
		strcpy(TempStatus, "");
		if (ModuleStatusCalls[Temp] != NULL)
		{
			ModuleStatusCalls[Temp](TempStatus);
			strcat(MyStatus, "|");
			strcat(MyStatus, TempStatus);
		}
	}
}

/**
	Get audio sample

	This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720

	All paks are scanned and the samples are merged/mixed
*/
extern "C" __declspec(dllexport) unsigned short VCC_PAKAPI_DEF_AUDIOSAMPLE(void)
{
	unsigned short TempSample = 0;
	for (int Temp = 0; Temp < MAXPAX; Temp++)
	{
		if (ModuleAudioSampleCalls[Temp] != NULL)
		{
			TempSample += ModuleAudioSampleCalls[Temp]();
		}
	}

	return(TempSample);
}

/**
	RESET - pass onto each Pak
*/
extern "C" __declspec(dllexport) unsigned char VCC_PAKAPI_DEF_RESET(void)
{
	ChipSelectSlot = SwitchSlot;
	SpareSelectSlot = SwitchSlot;

	for (int Temp = 0; Temp<MAXPAX; Temp++)
	{
		BankedCartOffset[Temp] = 0; // Do I need to keep independant selects?

		if (ModuleResetCalls[Temp] != NULL)
		{
			ModuleResetCalls[Temp]();
		}
	}
	PakSetCart(0);

	if (CartForSlot[SpareSelectSlot] == 1)
	{
		PakSetCart(1);
	}

	return(NULL);
}

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_SETINIPATH(char *IniFilePath)
{
	strcpy(IniFile, IniFilePath);
	LoadConfig();
}

/**
*/
extern "C" __declspec(dllexport) void VCC_PAKAPI_DEF_SETCART(vccapi_setcart_t Pointer)
{
	PakSetCart = Pointer;
}

/****************************************************************************/
/**
	Main entry point for the DLL (Windows)
*/
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpReserved)  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH) //Clean Up 
	{
		// save configuration
		WriteConfig();

		// clean up each pak
		for (int Temp = 0; Temp < MAXPAX; Temp++)
		{
			UnloadModule(Temp);
		}

		return(1);
	}

	g_hinstDLL = hinstDLL;

	return(1);
}

/****************************************************************************/
