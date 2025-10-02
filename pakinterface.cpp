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

#include <Windows.h>
#include <windowsx.h>
#include "commdlg.h"
#include <stdio.h>
#include <process.h>
#include "defines.h"
#include "tcc1014mmu.h"
#include "tcc1014registers.h"
#include "ModuleDefs.h"
#include "CartridgeMenu.h"
#include "pakinterface.h"
#include "config.h"
#include "Vcc.h"
#include "mc6821.h"
#include "logger.h"
#include "fileops.h"
#include "DialogOps.h"
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
static HINSTANCE hinstLib = nullptr;

static void (*GetModuleName)(char *,char *,CARTMENUCALLBACK)=nullptr;
static void (*ConfigModule)(unsigned char)=nullptr;
static void (*SetInteruptCallPointer)(PAKINTERUPT)=nullptr;
static void (*DmaMemPointer) (MEMREAD8,MEMWRITE8)=nullptr;
static void (*HeartBeat)()=nullptr;
static void (*PakPortWrite)(unsigned char,unsigned char)=nullptr;
static unsigned char (*PakPortRead)(unsigned char)=nullptr;
static void (*PakMemWrite8)(unsigned char,unsigned short)=nullptr;
static unsigned char (*PakMemRead8)(unsigned short)=nullptr;
static void (*ModuleStatus)(char *)=nullptr;
static unsigned short (*ModuleAudioSample)()=nullptr;
static void (*ModuleReset) ()=nullptr;
static void (*SetIniPath) (const char *)=nullptr;
static void (*PakSetCart)(SETCART)=nullptr;
static char PakPath[MAX_PATH] = "";
static char PakName[MAX_PATH] = "";

static char Did=0;
int FileID(const char *);

static HMENU hVccMenu = nullptr;
static bool CartMenuCreated = false;

static 	char Modname[MAX_PATH]="Blank";

void PakTimer()
{
	if (HeartBeat != nullptr)
		HeartBeat();
	return;
}

void ResetBus()
{
	BankedCartOffset=0;
	if (ModuleReset !=nullptr)
		ModuleReset();
	return;
}

void GetModuleStatus(SystemState *SMState)
{
	if (ModuleStatus!=nullptr)
		ModuleStatus(SMState->StatusLine);
	else
		sprintf(SMState->StatusLine,"");
	return;
}

unsigned char PackPortRead (unsigned char port)
{
	if (PakPortRead != nullptr)
		return(PakPortRead(port));

	return 0;
}

void PackPortWrite(unsigned char Port,unsigned char Data)
{
	if (PakPortWrite != nullptr)
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
	if (PakMemRead8!=nullptr)
		return(PakMemRead8(Address&32767));
	if (ExternalRomBuffer!=nullptr)
		return(ExternalRomBuffer[(Address & 32767)+BankedCartOffset]);
	return 0;
}

void PackMem8Write(unsigned short Address,unsigned char Value)
{
	if (PakMemWrite8!=nullptr)
		PakMemWrite8(Address&32767,Value);
	if (ExternalRomBuffer!=nullptr)
		ExternalRomBuffer[(Address & 32767)+BankedCartOffset] = Value;
	return;
}

// Convert PAK interrupt assert to CPU assert or Gime assert.
void (PakAssertInterupt) (unsigned char interrupt, unsigned char source)
{
	(void) source; // not used

	switch (interrupt) {
	case INT_CART:
		GimeAssertCartInterupt();
		break;
	case INT_NMI:
		CPUAssertInterupt(IS_NMI, INT_NMI);
		break;
	}
}

unsigned short PackAudioSample()
{
	if (ModuleAudioSample !=nullptr)
		return(ModuleAudioSample());
	return 0;
}

// Create first two entries for cartridge menu.
void BeginCartMenu()
{
	CartMenu.add("", MID_BEGIN, MIT_Head, 0);
	CartMenu.add("Cartridge",MID_ENTRY,MIT_Head);
	if (hinstLib) {
		char tmp[64];
		snprintf(tmp,64,"Eject %s",PakName);
		CartMenu.add(tmp,ControlId(2),MIT_Slave);
	} else {
		CartMenu.add("Load Cart",ControlId(1),MIT_Slave);
	}
	CartMenu.add("",MID_FINISH,MIT_Head);
}

// Callback for loaded cart DLLs. First two entries are reserved
void CartMenuCallBack(const char *name, int menu_id, int type)
{
	CartMenu.add(name, menu_id, (MenuItemType) type, 2);
}

int LoadCart()
{
	char inifile[MAX_PATH];
	GetIniFilePath(inifile);
	GetPrivateProfileString("DefaultPaths", "PakPath", "", PakPath, MAX_PATH, inifile);
	FileDialog dlg;
	dlg.setTitle(TEXT("Load Program Pack"));
	dlg.setInitialDir(PakPath);
	dlg.setFilter("DLL Packs\0*.dll\0Rom Packs\0*.ROM;*.ccc;*.pak\0\0");
	dlg.setFlags(OFN_FILEMUSTEXIST);
	if (dlg.show()) {
		if (InsertModule(dlg.path()) == 0) {
			dlg.getdir(PakPath);
			WritePrivateProfileString("DefaultPaths", "PakPath", PakPath, inifile);
			return 0;
		}
	}
	return 1;
}

// Insert Module returns 0 on success
int InsertModule (const char *ModulePath)
{
	char CatNumber[MAX_LOADSTRING]="";
	char Temp[MAX_LOADSTRING]="";
	char String[1024]="";
	char TempIni[MAX_PATH]="";
	unsigned char FileType=0;

	FileType=FileID(ModulePath);

	switch (FileType)
	{
	case 0:		//File doesn't exist
		return NOMODULE;
		break;

	case 2:		//File is a ROM image
		UnloadDll();
		load_ext_rom(ModulePath);

		strncpy(Modname,ModulePath,MAX_PATH);
		PathStripPath(Modname);
		BeginCartMenu();

		// Reset if enabled
		EmuState.ResetPending = 2;
		SetCart(1);
		return NOMODULE;
	break;

	case 1:		//File is a DLL
		UnloadDll();
		hinstLib=nullptr;
		hinstLib = LoadLibrary(ModulePath);
		_DLOG("pak:LoadLibrary %s %d\n",ModulePath,hinstLib);
		if (hinstLib == nullptr)
			return NOMODULE;

		strncpy(PakName,ModulePath,MAX_PATH);
		PathStripPath(PakName);
		BeginCartMenu();

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
		if (GetModuleName == nullptr)
		{
			FreeLibrary(hinstLib);
			_DLOG("pak:err FreeLibrary %d %d\n",hinstLib,rc);
			hinstLib=nullptr;
			return NOTVCC;
		}
		BankedCartOffset=0;
		if (DmaMemPointer!=nullptr)
			DmaMemPointer(MemRead8,MemWrite8);
		if (SetInteruptCallPointer!=nullptr)
			SetInteruptCallPointer(PakAssertInterupt);
		GetModuleName(Modname,CatNumber,CartMenuCallBack);  //Instanciate the menus HERE
		sprintf(Temp,"Configure %s",Modname);

		strcat(String,"Module Name: ");
		strcat(String,Modname);
		strcat(String,"\n");
		if (ConfigModule!=nullptr)
		{
			ModualParms|=1;
			strcat(String,"Has Configurable options\n");
		}
		if (PakPortWrite!=nullptr)
		{
			ModualParms|=2;
			strcat(String,"Is IO writable\n");
		}
		if (PakPortRead!=nullptr)
		{
			ModualParms|=4;
			strcat(String,"Is IO readable\n");
		}
		if (SetInteruptCallPointer!=nullptr)
		{
			ModualParms|=8;
			strcat(String,"Generates Interupts\n");
		}
		if (DmaMemPointer!=nullptr)
		{
			ModualParms|=16;
			strcat(String,"Generates DMA Requests\n");
		}
		if (HeartBeat!=nullptr)
		{
			ModualParms|=32;
			strcat(String,"Needs Heartbeat\n");
		}
		if (ModuleAudioSample!=nullptr)
		{
			ModualParms|=64;
			strcat(String,"Analog Audio Outputs\n");
		}
		if (PakMemWrite8!=nullptr)
		{
			ModualParms|=128;
			strcat(String,"Needs ChipSelect Write\n");
		}
		if (PakMemRead8!=nullptr)
		{
			ModualParms|=256;
			strcat(String,"Needs ChipSelect Read\n");
		}
		if (ModuleStatus!=nullptr)
		{
			ModualParms|=512;
			strcat(String,"Returns Status\n");
		}
		if (ModuleReset!=nullptr)
		{
			ModualParms|=1024;
			strcat(String,"Needs Reset Notification\n");
		}
		if (SetIniPath!=nullptr)
		{
			ModualParms|=2048;
			GetIniFilePath(TempIni);
			SetIniPath(TempIni);
		}
		if (PakSetCart!=nullptr)
		{
			ModualParms|=4096;
			strcat(String,"Can Assert CART\n");
			PakSetCart(SetCart);
		}
		strcpy(DllPath,ModulePath);
		EmuState.ResetPending=2;

		return 0;
		break;
	}
	return NOMODULE;
}

/**
Load a ROM pack
return total bytes loaded, or 0 on failure
*/
int load_ext_rom(const char *filename)
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
		MessageBox(nullptr, "cant allocate ram", "Ok", 0);
		return 0;
	}

	// Open the ROM file, fail if unable to
	FILE *rom_handle = fopen(filename, "rb");
	if (rom_handle == nullptr)
		return 0;

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

void UnloadDll()
{
	GetModuleName=nullptr;
	ConfigModule=nullptr;
	PakPortWrite=nullptr;
	PakPortRead=nullptr;
	SetInteruptCallPointer=nullptr;
	DmaMemPointer=nullptr;
	HeartBeat=nullptr;
	PakMemWrite8=nullptr;
	PakMemRead8=nullptr;
	ModuleStatus=nullptr;
	ModuleAudioSample=nullptr;
	ModuleReset=nullptr;
	int rc = FreeLibrary(hinstLib);
	_DLOG("pak:UnloadDll FreeLibrary %d %d\n",hinstLib,rc);
	hinstLib=nullptr;

	BeginCartMenu();
	return;
}

void GetCurrentModule(char *DefaultModule)
{
	strcpy(DefaultModule,DllPath);
	return;
}

void UpdateBusPointer()
{
	if (SetInteruptCallPointer!=nullptr)
		SetInteruptCallPointer(PakAssertInterupt);
	return;
}

void UnloadPack()
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
	BeginCartMenu();
	return;
}

int FileID(const char *Filename)
{
	FILE *DummyHandle=nullptr;
	char Temp[3]="";
	DummyHandle=fopen(Filename,"rb");
	if (DummyHandle==nullptr)
		return 0;	//File Doesn't exist

	Temp[0]=fgetc(DummyHandle);
	Temp[1]=fgetc(DummyHandle);
	Temp[2]=0;
	fclose(DummyHandle);
	if (strcmp(Temp,"MZ")==0)
		return 1;	//DLL File
	return 2;		//Rom Image
}

// CartMenuActivated is called from VCC main when a cartridge menu item is clicked.
void CartMenuActivated(unsigned int MenuID)
{
	switch (MenuID)
	{
	case 1:
		LoadPack();
		break;
	case 2:
		UnloadPack();
		break;
	default:
		if (ConfigModule !=nullptr) {
			ConfigModule(MenuID);
		}
		break;
	}
	return;
}


