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

#include <windows.h>
#include <windowsx.h>
#include "commdlg.h"
#include <stdio.h>
#include <process.h>
#include "defines.h"
#include "tcc1014mmu.h"
#include "pakinterface.h"
#include "config.h"
#include "Vcc.h"
#include "mc6821.h"
#include "logger.h"
#include "fileops.h"
#define HASCONFIG		1
#define HASIOWRITE		2
#define HASIOREAD		4
#define NEEDSCPUIRQ		8
#define DOESDMA			16
#define NEEDHEARTBEAT	32
#define ANALOGAUDIO		64
#define CSWRITE			128
#define CSREAD			256
#define RETURNSSTATUS	512
#define CARTRESET		1024
#define SAVESINI		2048
#define ASSERTCART		4096

// Storage for Pak ROMs
static uint8_t *ExternalRomBuffer = nullptr; 
static bool RomPackLoaded = false;

extern SystemState EmuState;
static unsigned int BankedCartOffset=0;
static char DllPath[256]="";
static unsigned short ModualParms=0;
static HINSTANCE hinstLib; 
static bool DialogOpen=false;
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef void (*GETNAME)(char *,char *,DYNAMICMENUCALLBACK); 
typedef void (*CONFIGIT)(unsigned char); 
typedef void (*HEARTBEAT) (void);
typedef unsigned char (*PACKPORTREAD)(unsigned char);
typedef void (*PACKPORTWRITE)(unsigned char,unsigned char);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*SETCART)(unsigned char);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*MODULESTATUS)(char *);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*SETCARTPOINTER)(SETCART);
typedef void (*SETINTERUPTCALLPOINTER) (ASSERTINTERUPT);
typedef unsigned short (*MODULEAUDIOSAMPLE)(void);
typedef void (*MODULERESET)(void);
typedef void (*SETINIPATH)(char *);

static void (*GetModuleName)(char *,char *,DYNAMICMENUCALLBACK)=NULL;
static void (*ConfigModule)(unsigned char)=NULL;
static void (*SetInteruptCallPointer) ( ASSERTINTERUPT)=NULL;
static void (*DmaMemPointer) (MEMREAD8,MEMWRITE8)=NULL;
static void (*HeartBeat)(void)=NULL;
static void (*PakPortWrite)(unsigned char,unsigned char)=NULL;
static unsigned char (*PakPortRead)(unsigned char)=NULL;
static void (*PakMemWrite8)(unsigned char,unsigned short)=NULL;
static unsigned char (*PakMemRead8)(unsigned short)=NULL;
static void (*ModuleStatus)(char *)=NULL;
static unsigned short (*ModuleAudioSample)(void)=NULL;
static void (*ModuleReset) (void)=NULL;
static void (*SetIniPath) (char *)=NULL;
static void (*PakSetCart)(SETCART)=NULL;


static char Did=0;
int FileID(char *);
typedef struct {
	char MenuName[512];
	int MenuId;
	int Type;
} Dmenu;

static Dmenu MenuItem[100];
static unsigned char MenuIndex=0;
static HMENU hMenu = NULL;
static HMENU hSubMenu[64] ;


static 	char Modname[MAX_PATH]="Blank";
void PakTimer(void)
{
	if (HeartBeat != NULL)
		HeartBeat();
	return;
}

void ResetBus(void)
{
	BankedCartOffset=0;
	if (ModuleReset !=NULL)
		ModuleReset();
	return;
}

void GetModuleStatus(SystemState *SMState)
{
	if (ModuleStatus!=NULL)
		ModuleStatus(SMState->StatusLine);
	else
		sprintf(SMState->StatusLine,"");
	return;
}

unsigned char PackPortRead (unsigned char port)
{
	if (PakPortRead != NULL)
		return(PakPortRead(port));
	else
		return(NULL);
}

void PackPortWrite(unsigned char Port,unsigned char Data)
{
	if (PakPortWrite != NULL)
	{
		PakPortWrite(Port,Data);
		return;
	}
	
	if ((Port == 0x40) && (RomPackLoaded == true)) {
		BankedCartOffset = (Data & 15) << 14;
	}

	return;
}

unsigned char PackMem8Read (unsigned short Address)
{
	if (PakMemRead8!=NULL)
		return(PakMemRead8(Address&32767));
	if (ExternalRomBuffer!=NULL)
		return(ExternalRomBuffer[(Address & 32767)+BankedCartOffset]);
	return(0);
}

void PackMem8Write(unsigned char Port,unsigned char Data)
{
	return;
}

unsigned short PackAudioSample(void)
{
	if (ModuleAudioSample !=NULL)
		return(ModuleAudioSample());
	return(NULL);
}

int LoadCart(void)
{
	OPENFILENAME ofn ;	
	char szFileName[MAX_PATH]="";
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = EmuState.WindowHandle;
	ofn.lpstrFilter = "Program Packs\0*.ROM;*.ccc;*.DLL\0\0";			// filter string
	ofn.nFilterIndex      = 1 ;							// current filter index
	ofn.lpstrFile         = szFileName ;				// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;					// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;						// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH ;					// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = NULL ;						// initial directory
	ofn.lpstrTitle        = TEXT("Load Program Pack") ;	// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	if ( GetOpenFileName (&ofn))
		if (!InsertModule (szFileName))
			return(0);
	return(1);
}

int InsertModule (char *ModulePath)
{
//	char Modname[MAX_LOADSTRING]="Blank";
	char CatNumber[MAX_LOADSTRING]="";
	char Temp[MAX_LOADSTRING]="";
	char String[1024]="";
	char TempIni[MAX_PATH]="";
	unsigned char FileType=0;
	FileType=FileID(ModulePath);


	switch (FileType)
	{
	case 0:		//File doesn't exist
		return(NOMODULE);
		break;

	case 2:		//File is a ROM image

		UnloadDll();
		load_ext_rom(ModulePath);
		strncpy(Modname,ModulePath,MAX_PATH);
		PathStripPath(Modname);
		DynamicMenuCallback( "",0, 0); //Refresh Menus
		DynamicMenuCallback( "",1, 0);
		EmuState.ResetPending=2;
		SetCart(1);
		return(NOMODULE);
	break;

	case 1:		//File is a DLL
		UnloadDll();
		hinstLib = LoadLibrary(ModulePath);
		if (hinstLib ==NULL)
			return(NOMODULE);
		SetCart(0);
		GetModuleName=(GETNAME)GetProcAddress(hinstLib, "ModuleName"); 
		ConfigModule=(CONFIGIT)GetProcAddress(hinstLib, "ModuleConfig");
		PakPortWrite=(PACKPORTWRITE) GetProcAddress(hinstLib, "PackPortWrite");
		PakPortRead=(PACKPORTREAD) GetProcAddress(hinstLib, "PackPortRead");
		SetInteruptCallPointer=(SETINTERUPTCALLPOINTER)GetProcAddress(hinstLib, "AssertInterupt");
		DmaMemPointer=(DMAMEMPOINTERS) GetProcAddress(hinstLib, "MemPointers");
		HeartBeat=(HEARTBEAT) GetProcAddress(hinstLib, "HeartBeat");
		PakMemWrite8=(MEMWRITE8) GetProcAddress(hinstLib, "PakMemWrite8");
		PakMemRead8=(MEMREAD8) 	GetProcAddress(hinstLib, "PakMemRead8");
		ModuleStatus=(MODULESTATUS) GetProcAddress(hinstLib, "ModuleStatus");
		ModuleAudioSample=(MODULEAUDIOSAMPLE) GetProcAddress(hinstLib, "ModuleAudioSample");
		ModuleReset=(MODULERESET) GetProcAddress(hinstLib, "ModuleReset");
		SetIniPath=(SETINIPATH) GetProcAddress(hinstLib,"SetIniPath");
		PakSetCart=(SETCARTPOINTER) GetProcAddress(hinstLib,"SetCart");
		if (GetModuleName == NULL)
		{
			FreeLibrary(hinstLib); 
			hinstLib=NULL;
			return(NOTVCC);
		}
		BankedCartOffset=0;
		if (DmaMemPointer!=NULL)
			DmaMemPointer(MemRead8,MemWrite8);
		if (SetInteruptCallPointer!=NULL)
			SetInteruptCallPointer(CPUAssertInterupt);

		GetModuleName(Modname,CatNumber,DynamicMenuCallback);  //Instanciate the menus from HERE!
		sprintf(Temp,"Configure %s",Modname);

		strcat(String,"Module Name: ");
		strcat(String,Modname);
		strcat(String,"\n");
		if (ConfigModule!=NULL)
		{
			ModualParms|=1;
			strcat(String,"Has Configurable options\n");
		}
		if (PakPortWrite!=NULL)
		{
			ModualParms|=2;
			strcat(String,"Is IO writable\n");
		}
		if (PakPortRead!=NULL)
		{
			ModualParms|=4;
			strcat(String,"Is IO readable\n");
		}
		if (SetInteruptCallPointer!=NULL)
		{
			ModualParms|=8;
			strcat(String,"Generates Interupts\n");
		}
		if (DmaMemPointer!=NULL)
		{
			ModualParms|=16;
			strcat(String,"Generates DMA Requests\n");
		}
		if (HeartBeat!=NULL)
		{
			ModualParms|=32;
			strcat(String,"Needs Heartbeat\n");
		}
		if (ModuleAudioSample!=NULL)
		{
			ModualParms|=64;
			strcat(String,"Analog Audio Outputs\n");
		}
		if (PakMemWrite8!=NULL)
		{
			ModualParms|=128;
			strcat(String,"Needs ChipSelect Write\n");
		}
		if (PakMemRead8!=NULL)
		{
			ModualParms|=256;
			strcat(String,"Needs ChipSelect Read\n");
		}
		if (ModuleStatus!=NULL)
		{
			ModualParms|=512;
			strcat(String,"Returns Status\n");
		}
		if (ModuleReset!=NULL)
		{
			ModualParms|=1024;
			strcat(String,"Needs Reset Notification\n");
		}
		if (SetIniPath!=NULL)
		{
			ModualParms|=2048;
			GetIniFilePath(TempIni);
			SetIniPath(TempIni);
		}
		if (PakSetCart!=NULL)
		{
			ModualParms|=4096;
			strcat(String,"Can Assert CART\n");
			PakSetCart(SetCart);
		}
		strcpy(DllPath,ModulePath);
		EmuState.ResetPending=2;
		return(0);
		break;
	}
	return(NOMODULE);
}

/**
Load a ROM pack
return total bytes loaded, or 0 on failure
*/
int load_ext_rom(char filename[MAX_PATH])
{
	constexpr size_t PAK_MAX_MEM = 0x40000;

	// If there is an existing ROM, ditch it
	if (ExternalRomBuffer != nullptr) {
		free(ExternalRomBuffer);
	}
	
	// Allocate memory for the ROM
	ExternalRomBuffer = (uint8_t*)malloc(PAK_MAX_MEM);

	// If memory was unable to be allocated, fail
	if (ExternalRomBuffer == nullptr) {
		MessageBox(0, "cant allocate ram", "Ok", 0);
		return 0;
	}
	
	// Open the ROM file, fail if unable to
	FILE *rom_handle = fopen(filename, "rb");
	if (rom_handle == nullptr) return 0;
	
	// Load the file, one byte at a time.. (TODO: Get size and read entire block)
	size_t index=0;
	while ((feof(rom_handle) == 0) && (index < PAK_MAX_MEM)) {
		ExternalRomBuffer[index++] = fgetc(rom_handle);
	}
	fclose(rom_handle);
	
	UnloadDll();
	BankedCartOffset=0;
	RomPackLoaded=true;
	
	return index;
}

void UnloadDll(void)
{
	if ((DialogOpen==true) & (EmuState.EmulationRunning==1))
	{
		MessageBox(0,"Close Configuration Dialog before unloading","Ok",0);
		return;
	}
	GetModuleName=NULL;
	ConfigModule=NULL;
	PakPortWrite=NULL;
	PakPortRead=NULL;
	SetInteruptCallPointer=NULL;
	DmaMemPointer=NULL;
	HeartBeat=NULL;
	PakMemWrite8=NULL;
	PakMemRead8=NULL;
	ModuleStatus=NULL;
	ModuleAudioSample=NULL;
	ModuleReset=NULL;
	if (hinstLib !=NULL)
		FreeLibrary(hinstLib); 
	hinstLib=NULL;
	DynamicMenuCallback( "",0, 0); //Refresh Menus
	DynamicMenuCallback( "",1, 0);
//	DynamicMenuCallback("",0,0);
	return;
}

void GetCurrentModule(char *DefaultModule)
{
	strcpy(DefaultModule,DllPath);
	return;
}

void UpdateBusPointer(void)
{
	if (SetInteruptCallPointer!=NULL)
		SetInteruptCallPointer(CPUAssertInterupt);
	return;
}

void UnloadPack(void)
{
	UnloadDll();
	strcpy(DllPath,"");
	strcpy(Modname,"Blank");
	RomPackLoaded=false;
	SetCart(0);
	
	if (ExternalRomBuffer != nullptr) {
		free(ExternalRomBuffer);
	}
	ExternalRomBuffer=nullptr;

	EmuState.ResetPending=2;
	DynamicMenuCallback( "",0, 0); //Refresh Menus
	DynamicMenuCallback( "",1, 0);
	return;
}

int FileID(char *Filename)
{
	FILE *DummyHandle=NULL;
	char Temp[3]="";
	DummyHandle=fopen(Filename,"rb");
	if (DummyHandle==NULL)
		return(0);	//File Doesn't exist

	Temp[0]=fgetc(DummyHandle);
	Temp[1]=fgetc(DummyHandle);
	Temp[2]=0;
	fclose(DummyHandle);
	if (strcmp(Temp,"MZ")==0)
		return(1);	//DLL File
	return(2);		//Rom Image 
}

void DynamicMenuActivated(unsigned char MenuItem)
{
	switch (MenuItem)
	{
	case 1:
		LoadPack();
		break;
	case 2:
		UnloadPack();
		break;
	default:
		if (ConfigModule !=NULL)
			ConfigModule(MenuItem);
		break;
	}
	return;
}

void DynamicMenuCallback( char *MenuName,int MenuId, int Type)
{
	char Temp[256]="";
	//MenuId=0 Flush Buffer MenuId=1 Done 
	switch (MenuId)
	{
		case 0:
			MenuIndex=0;
			DynamicMenuCallback( "Cartridge",6000,HEAD);	//Recursion is fun
			DynamicMenuCallback( "Load Cart",5001,SLAVE);
			sprintf(Temp,"Eject Cart: ");
			strcat(Temp,Modname);
			DynamicMenuCallback( Temp,5002,SLAVE);
		break;

		case 1:
			RefreshDynamicMenu();
		break;

		default:
			strcpy(MenuItem[MenuIndex].MenuName,MenuName);
			MenuItem[MenuIndex].MenuId=MenuId;
			MenuItem[MenuIndex].Type=Type;
			MenuIndex++;
		break;	
	}
	return;
}

void RefreshDynamicMenu(void)
{
	MENUITEMINFO	Mii;
	char MenuTitle[32]="Cartridge";
	unsigned char TempIndex=0,Index=0;
	static HWND hOld;
	int SubMenuIndex=0;
	if ((hMenu==NULL) | (EmuState.WindowHandle != hOld))
		hMenu=GetMenu(EmuState.WindowHandle);
	else
		DeleteMenu(hMenu,3,MF_BYPOSITION);

	hOld=EmuState.WindowHandle;
	hSubMenu[SubMenuIndex]=CreatePopupMenu();
	memset(&Mii,0,sizeof(MENUITEMINFO));
	Mii.cbSize= sizeof(MENUITEMINFO);
	Mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
	Mii.fType = MFT_STRING;
	Mii.wID = 4999;
	Mii.hSubMenu = hSubMenu[SubMenuIndex];
	Mii.dwTypeData = MenuTitle;
	Mii.cch=strlen(MenuTitle);
	InsertMenuItem(hMenu,3,TRUE,&Mii);
	SubMenuIndex++;	
	for (TempIndex=0;TempIndex<MenuIndex;TempIndex++)
	{
		if (strlen(MenuItem[TempIndex].MenuName) ==0)
			MenuItem[TempIndex].Type=STANDALONE;

		//Create Menu item in title bar if no exist already
		switch (MenuItem[TempIndex].Type)
		{
		case HEAD:
				SubMenuIndex++;
				hSubMenu[SubMenuIndex]=CreatePopupMenu();
				memset(&Mii,0,sizeof(MENUITEMINFO));
				Mii.cbSize= sizeof(MENUITEMINFO);
				Mii.fMask = MIIM_TYPE | MIIM_SUBMENU | MIIM_ID;
				Mii.fType = MFT_STRING;
				Mii.wID = MenuItem[TempIndex].MenuId;
				Mii.hSubMenu =hSubMenu[SubMenuIndex];
				Mii.dwTypeData = MenuItem[TempIndex].MenuName;
				Mii.cch=strlen(MenuItem[TempIndex].MenuName);
				InsertMenuItem(hSubMenu[0],0,FALSE,&Mii);		

			break;

		case SLAVE:
				memset(&Mii,0,sizeof(MENUITEMINFO));
				Mii.cbSize= sizeof(MENUITEMINFO);
				Mii.fMask = MIIM_TYPE |  MIIM_ID;
				Mii.fType = MFT_STRING;
				Mii.wID = MenuItem[TempIndex].MenuId;
				Mii.hSubMenu = hSubMenu[SubMenuIndex];
				Mii.dwTypeData = MenuItem[TempIndex].MenuName;
				Mii.cch=strlen(MenuItem[TempIndex].MenuName);
				InsertMenuItem(hSubMenu[SubMenuIndex],0,FALSE,&Mii);


		break;

		case STANDALONE:
			if (strlen(MenuItem[TempIndex].MenuName) ==0)
			{
				memset(&Mii,0,sizeof(MENUITEMINFO));
				Mii.cbSize= sizeof(MENUITEMINFO);
				Mii.fMask = MIIM_TYPE |  MIIM_ID;
				Mii.fType = MF_SEPARATOR; 
			//	Mii.fType = MF_MENUBARBREAK;
			//	Mii.fType = MFT_STRING;
				Mii.wID = MenuItem[TempIndex].MenuId;
				Mii.hSubMenu = hMenu;
				Mii.dwTypeData = MenuItem[TempIndex].MenuName;
				Mii.cch=strlen(MenuItem[TempIndex].MenuName);
				InsertMenuItem(hSubMenu[0],0,FALSE,&Mii);
			}
			else
			{
				memset(&Mii,0,sizeof(MENUITEMINFO));
				Mii.cbSize= sizeof(MENUITEMINFO);
				Mii.fMask = MIIM_TYPE |  MIIM_ID;
				Mii.fType = MFT_STRING;
				Mii.wID = MenuItem[TempIndex].MenuId;
				Mii.hSubMenu = hMenu;
				Mii.dwTypeData = MenuItem[TempIndex].MenuName;
				Mii.cch=strlen(MenuItem[TempIndex].MenuName);
				InsertMenuItem(hSubMenu[0],0,FALSE,&Mii);
			}
		break;
		}
	}
	DrawMenuBar(EmuState.WindowHandle);
	return;
}

