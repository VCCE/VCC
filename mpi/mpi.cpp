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
// This is an expansion module for the Vcc Emulator. It simulated the functions of the TRS-80 Multi-Pak Interface

#include <windows.h>
#include <iostream>
#include "stdio.h"
#include "resource.h"
#include <commctrl.h>
#include "mpi.h"
#include "../fileops.h"
#include "../MachineDefs.h"
#include "../logger.h"

#define MAXPAX 4

using namespace std;
static void (*AssertInt)(unsigned char, unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short)=NULL;
static void (*MemWrite8)(unsigned char,unsigned short)=NULL;

static void (*PakSetCart)(unsigned char)=NULL;
static HINSTANCE g_hinstDLL=NULL;
static char CatNumber[MAXPAX][MAX_LOADSTRING]={"","","",""};
static char SlotLabel[MAXPAX][MAX_LOADSTRING*2]={"Empty","Empty","Empty","Empty"};
//static unsigned char PersistPaks = 0;
//static unsigned char DisableSCS = 0;
static char ModulePaths[MAXPAX][MAX_PATH]={"","","",""};
static char ModuleNames[MAXPAX][MAX_LOADSTRING]={"Empty","Empty","Empty","Empty"};
static unsigned char *ExtRomPointers[MAXPAX]={NULL,NULL,NULL,NULL};
static unsigned int BankedCartOffset[MAXPAX]={0,0,0,0};
static char IniFile[MAX_PATH]="";
static char MPIPath[MAX_PATH];


//**************************************************************
//Array of fuction pointer for each Slot
static void (*GetModuleNameCalls[MAXPAX])(char *,char *,DYNAMICMENUCALLBACK)={NULL,NULL,NULL,NULL};
static void (*ConfigModuleCalls[MAXPAX])(unsigned char)={NULL,NULL,NULL,NULL};
static void (*HeartBeatCalls[MAXPAX])(void)={NULL,NULL,NULL,NULL};
static void (*PakPortWriteCalls[MAXPAX])(unsigned char,unsigned char)={NULL,NULL,NULL,NULL};
static unsigned char (*PakPortReadCalls[MAXPAX])(unsigned char)={NULL,NULL,NULL,NULL};
static void (*PakMemWrite8Calls[MAXPAX])(unsigned char,unsigned short)={NULL,NULL,NULL,NULL};
static unsigned char (*PakMemRead8Calls[MAXPAX])(unsigned short)={NULL,NULL,NULL,NULL};
static void (*ModuleStatusCalls[MAXPAX])(char *)={NULL,NULL,NULL,NULL};
static unsigned short (*ModuleAudioSampleCalls[MAXPAX])(void)={NULL,NULL,NULL,NULL};
static void (*ModuleResetCalls[MAXPAX]) (void)={NULL,NULL,NULL,NULL};
//Set callbacks for the DLL to call
static void (*SetInteruptCallPointerCalls[MAXPAX]) ( PAKINTERUPT)={NULL,NULL,NULL,NULL};
static void (*DmaMemPointerCalls[MAXPAX]) (MEMREAD8,MEMWRITE8)={NULL,NULL,NULL,NULL};
//MenuName,int MenuId, int Type)

static char MenuName0[64][512],MenuName1[64][512],MenuName2[64][512],MenuName3[64][512];
static int MenuId0[64],MenuId1[64],MenuId2[64],MenuId3[64];
static int Type0[64],Type1[64],Type2[64],Type3[64];
static int MenuCount[MAXPAX]={0};

void CenterDialog(HWND);
void SetCartSlot0(unsigned char);
void SetCartSlot1(unsigned char);
void SetCartSlot2(unsigned char);
void SetCartSlot3(unsigned char);
void BuildDynaMenu(void);
void DynamicMenuCallback0(char *,int, int);
void DynamicMenuCallback1(char *,int, int);
void DynamicMenuCallback2(char *,int, int);
void DynamicMenuCallback3(char *,int, int);
static unsigned char CartForSlot[MAXPAX]={0};
static void (*SetCarts[MAXPAX])(unsigned char)={SetCartSlot0,SetCartSlot1,SetCartSlot2,SetCartSlot3};
static void (*DynamicMenuCallbackCalls[MAXPAX])(char *,int, int)={DynamicMenuCallback0,DynamicMenuCallback1,DynamicMenuCallback2,DynamicMenuCallback3};
static void (*SetCartCalls[MAXPAX])(SETCART)={NULL};

static void (*SetIniPathCalls[MAXPAX]) (char *)={NULL};
static void (*DynamicMenuCallback)( char *,int, int)=NULL;
//***************************************************************
static HINSTANCE hinstLib[MAXPAX]={NULL};
static unsigned char ChipSelectSlot=3,SpareSelectSlot=3,SwitchSlot=3,SlotRegister=255;
HWND hConfDlg=NULL;

//Function Prototypes for this module
LRESULT CALLBACK MpiConfigDlg(HWND,UINT,WPARAM,LPARAM);

unsigned char MountModule(unsigned char,const char *);
void UnloadModule(unsigned char);
void LoadCartDLL(unsigned char,char *);
void LoadConfig(void);
void WriteConfig(void);
void ReadModuleParms(unsigned char,char *);
int FileID(const char *);

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up
	{
		WriteConfig();
		for (int slot=0;slot<MAXPAX;slot++) UnloadModule(slot);
		if (hConfDlg)
			DestroyWindow(hConfDlg);
		hConfDlg = NULL;
		return(1);
	}
	g_hinstDLL=hinstDLL;
	return(1);
}

void MemWrite(unsigned char Data,unsigned short Address)
{
	MemWrite8(Data,Address);
	return;
}

unsigned char MemRead(unsigned short Address)
{
	return(MemRead8(Address));
}

extern "C"
{
	__declspec(dllexport) void ModuleName(char *ModName,char *CatNumber,DYNAMICMENUCALLBACK Temp)
	{
		LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
		LoadString(g_hinstDLL,IDS_CATNUMBER,CatNumber, MAX_LOADSTRING);
		DynamicMenuCallback =Temp;
		return ;
	}
}

extern "C"
{
	// MenuID is 1 thru 100
	__declspec(dllexport) void ModuleConfig(unsigned char MenuID)
	{
		// Vcc subtracts 5000 from the MenuId to get MenuID
		// MPI config MenuId is 5019 so 19 is used here.
		if (MenuID == 19) {
			if (hConfDlg)
				DestroyWindow(hConfDlg);
			CreateDialog(g_hinstDLL, (LPCTSTR)IDD_DIALOG1,
						GetActiveWindow(), (DLGPROC)MpiConfigDlg);
			ShowWindow(hConfDlg,1);
		}

		//Configs for loaded carts
		if ( (MenuID>=20) & (MenuID <=40) )
			ConfigModuleCalls[0](MenuID-20);

		if ( (MenuID>40) & (MenuID <=60) )
			ConfigModuleCalls[1](MenuID-40);

		if ( (MenuID>60) & (MenuID <=80) )
			ConfigModuleCalls[2](MenuID-60);

		if ( (MenuID>80) & (MenuID <=100) )
			ConfigModuleCalls[3](MenuID-80);
		return;
	}

}

// This captures the Function transfer point for the CPU assert interupt
extern "C"
{
	__declspec(dllexport) void AssertInterupt(PAKINTERUPT Dummy)
	{
		AssertInt=Dummy;
		for (int slot=0;slot<MAXPAX;slot++) {
			if(SetInteruptCallPointerCalls[slot] !=NULL)
				SetInteruptCallPointerCalls[slot](AssertInt);
		}
		return;
	}
}

extern "C"
{
	__declspec(dllexport) void PackPortWrite(unsigned char Port,unsigned char Data)
	{
		if (Port == 0x7F) //Addressing the Multi-Pak
		{
			SpareSelectSlot= (Data & 3);          //SCS
			ChipSelectSlot= ( (Data & 0x30)>>4);  //CTS
			SlotRegister=Data;
			PakSetCart(0);
			if (CartForSlot[SpareSelectSlot]==1)
				PakSetCart(1);
			return;
		}

		// Skip write disk ports 0x40 - 0x5F unless SCS is set
		bool DiskPort = (Port > 0x3F) && (Port < 0x60);
		if (DiskPort)
		{
			BankedCartOffset[SpareSelectSlot]=(Data & 15)<<14;
			if ( PakPortWriteCalls[SpareSelectSlot] != NULL)
				PakPortWriteCalls[SpareSelectSlot](Port,Data);
		}
		else
			for (int slot=0;slot<MAXPAX;slot++)
				if (PakPortWriteCalls[slot] != NULL)
					PakPortWriteCalls[slot](Port,Data);
		return;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
	{
		if (Port == 0x7F) { // Self
			SlotRegister&=0xCC;
			SlotRegister|=(SpareSelectSlot | (ChipSelectSlot<<4));
			return(SlotRegister);
		}

		// Skip disk read ports 0x40 - 0x5F unless SCS is set
		bool DiskPort = (Port > 0x3F) && (Port < 0x60);
		if (DiskPort) {
			if ( PakPortReadCalls[SpareSelectSlot] != NULL)
				return(PakPortReadCalls[SpareSelectSlot](Port));
			else
				return(0);
		}

		for (int slot=0;slot<MAXPAX;slot++) {
			if ( PakPortReadCalls[slot] !=NULL) {
				//Return value from first module that returns non zero
				unsigned char data=PakPortReadCalls[slot](Port);
				if (data != 0)
					return(data);
			}
		}
		return(0);
	}
}

extern "C"
{
	__declspec(dllexport) void HeartBeat(void)
	{
		for (int slot=0;slot<MAXPAX;slot++)
			if (HeartBeatCalls[slot] != NULL)
				HeartBeatCalls[slot]();
		return;
	}
}


//This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.
extern "C"
{
	__declspec(dllexport) void MemPointers(MEMREAD8 Temp1,MEMWRITE8 Temp2)
	{
		MemRead8=Temp1;
		MemWrite8=Temp2;
		return;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PakMemRead8(unsigned short Address)
	{
		if (ExtRomPointers[ChipSelectSlot] != NULL)
			return(ExtRomPointers[ChipSelectSlot][(Address & 32767)+BankedCartOffset[ChipSelectSlot]]); //Bank Select ???
		if (PakMemRead8Calls[ChipSelectSlot] != NULL)
			return(PakMemRead8Calls[ChipSelectSlot](Address));
		return(NULL);
	}
}

extern "C"
{
	__declspec(dllexport) void PakMemWrite8(unsigned char Data,unsigned short Address)
	{

		return;
	}
}

extern "C"
{
	__declspec(dllexport) void ModuleStatus(char *MyStatus)
	{
		char TempStatus[64]="";
		sprintf(MyStatus,"MPI:%d,%d",ChipSelectSlot+1,SpareSelectSlot+1);
		for (int slot=0;slot<MAXPAX;slot++)
		{
			strcpy(TempStatus,"");
			if (ModuleStatusCalls[slot] != NULL)
			{
				ModuleStatusCalls[slot](TempStatus);
				strcat(MyStatus," | ");
				strcat(MyStatus,TempStatus);
			}
		}
		return ;
	}
}

// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720
extern "C"
{
	__declspec(dllexport) unsigned short ModuleAudioSample(void)
	{
		unsigned short TempSample=0;
		for (int slot=0;slot<MAXPAX;slot++)
			if (ModuleAudioSampleCalls[slot] != NULL)
				TempSample+=ModuleAudioSampleCalls[slot]();

		return(TempSample) ;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char ModuleReset (void)
	{
		ChipSelectSlot=SwitchSlot;
		SpareSelectSlot=SwitchSlot;
		for (int slot=0;slot<MAXPAX;slot++)
		{
			BankedCartOffset[slot]=0; //Do I need to keep independant selects?

			if (ModuleResetCalls[slot] !=NULL)
				ModuleResetCalls[slot]();
		}
		PakSetCart(0);
		if (CartForSlot[SpareSelectSlot]==1)
			PakSetCart(1);
		return(NULL);
	}
}

extern "C"
{
	__declspec(dllexport) void SetIniPath (char *IniFilePath)
	{
		strcpy(IniFile,IniFilePath);
		LoadConfig();
		return;
	}
}

//void AssertInterupt(Interrupt interrupt)
//{
//     AssertInt(IS_PIA1_CART, interrupt);
//}

extern "C"
{
	__declspec(dllexport) void SetCart(SETCART Pointer)
	{

		PakSetCart=Pointer;
		return;
	}
}

void CenterDialog(HWND hDlg)
{
	RECT rPar, rDlg;
	GetWindowRect(GetParent(hDlg), &rPar);
	GetWindowRect(hDlg, &rDlg);
	int x = rPar.left + (rPar.right - rPar.left - (rDlg.right - rDlg.left)) / 2;
	int y = rPar.top + (rPar.bottom - rPar.top - (rDlg.bottom - rDlg.top)) / 2;
	SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK MpiConfigDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned short EDITBOXS[4]={IDC_EDIT1,IDC_EDIT2,IDC_EDIT3,IDC_EDIT4};
	unsigned short RADIOBTN[4]={IDC_SELECT1,IDC_SELECT2,IDC_SELECT3,IDC_SELECT4};
	unsigned short INSERTBTN[4]={IDC_INSERT1,IDC_INSERT2,IDC_INSERT3,IDC_INSERT4};
	unsigned short CONFIGBTN[4]={ID_CONFIG1,ID_CONFIG2,ID_CONFIG3,ID_CONFIG4};

	char ConfigText[1024]="";

	switch (message) {
	case WM_CLOSE:
//		WriteConfig();
		EndDialog(hDlg, LOWORD(wParam));
		hConfDlg=NULL;
//		PersistPaks = (unsigned char)SendDlgItemMessage(hDlg,IDC_PERSIST_PAK,BM_GETCHECK,0,0);
//		DisableSCS = (unsigned char)SendDlgItemMessage(hDlg,IDC_SCS_DISABLE,BM_GETCHECK,0,0);
		return TRUE;
		break;
	case WM_INITDIALOG:
//LoadConfig();
		CenterDialog(hDlg);
		hConfDlg=hDlg;
		for (int slot=0;slot<MAXPAX;slot++) {
			SendDlgItemMessage(hDlg,EDITBOXS[slot],WM_SETTEXT,0,(LPARAM)SlotLabel[slot]);
		}
		ReadModuleParms(SwitchSlot,ConfigText);
		SendDlgItemMessage(hDlg,IDC_MODINFO,WM_SETTEXT,0,(LPARAM)ConfigText);
		for (int slot=0;slot<MAXPAX;slot++) {
			if (SwitchSlot==slot)
				SendDlgItemMessage(hDlg, RADIOBTN[slot], BM_SETCHECK, 1, 0);
			else
				SendDlgItemMessage(hDlg, RADIOBTN[slot], BM_SETCHECK, 0, 0);
		}
		return TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SELECT1:
		case IDC_SELECT2:
		case IDC_SELECT3:
		case IDC_SELECT4:
			for (int slot=0;slot<MAXPAX;slot++) {
				if (RADIOBTN[slot] == LOWORD(wParam)) {
					SwitchSlot = slot;
					SendDlgItemMessage(hDlg, RADIOBTN[slot], BM_SETCHECK, 1, 0);
				} else {
					SendDlgItemMessage(hDlg, RADIOBTN[slot], BM_SETCHECK, 0, 0);
				}
			}
			SpareSelectSlot = SwitchSlot;
			ChipSelectSlot = SwitchSlot;
			ReadModuleParms(SwitchSlot,ConfigText);
			SendDlgItemMessage(hDlg,IDC_MODINFO,WM_SETTEXT,0,(LPARAM)(LPCSTR)ConfigText);
			PakSetCart(0);
			if (CartForSlot[SpareSelectSlot]==1)
				PakSetCart(1);
			return TRUE;
			break;
		} //End switch LOWORD

		for (int slot=0;slot<MAXPAX;slot++) {
			if ( LOWORD(wParam) == INSERTBTN[slot] ) {
				LoadCartDLL(slot,ModulePaths[slot]);
				for (int slot=0;slot<MAXPAX;slot++)
					SendDlgItemMessage(hDlg,EDITBOXS[slot],WM_SETTEXT,0,(LPARAM)(LPCSTR)SlotLabel[slot]);
			}
		}

		for (int slot=0;slot<MAXPAX;slot++) {
			if ( LOWORD(wParam) == CONFIGBTN[slot] ) {
				if (ConfigModuleCalls[slot] != NULL)
					ConfigModuleCalls[slot](NULL);
			}
		}
		return TRUE;
		break;
	}
    return FALSE;
}

unsigned char MountModule(unsigned char Slot,const char *ModuleName)
{
	PrintLogC("MountModule slot %d '%s'\n",Slot, ModuleName);

	unsigned int index=0;
	FILE *rom_handle;
	if (Slot>3)
		return(0);

	// Copy ModuleName otherwise UnloadModule() will change it.
	char MountName[MAX_PATH]="";
	strcpy(MountName,ModuleName);

	unsigned char ModuleType = FileID(MountName);

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
		rom_handle=fopen(MountName,"rb");
		if (rom_handle==NULL)
		{
			MessageBox(0,"File handle is NULL","Error",0);
			return(0);
		}
		while ((feof(rom_handle)==0) & (index<0x40000))
			ExtRomPointers[Slot][index++]=fgetc(rom_handle);
		fclose(rom_handle);
		strcpy(ModulePaths[Slot],MountName);
		PathStripPath(MountName);
		PathRemoveExtension(MountName);
		strcpy(ModuleNames[Slot],MountName);
		strcpy(SlotLabel[Slot],MountName); //JF
		CartForSlot[Slot]=1;

//		if (CartForSlot[SpareSelectSlot]==1)
//			PakSetCart(1);
		return(1);
	break;

	case 1:	//DLL File
		UnloadModule(Slot);
		strcpy(ModulePaths[Slot],MountName);
		hinstLib[Slot] = LoadLibrary(MountName);
		if (hinstLib[Slot]==NULL)
			return(0);	//Error Can't open File
		GetModuleNameCalls[Slot]=(GETNAME)GetProcAddress(hinstLib[Slot], "ModuleName");
		ConfigModuleCalls[Slot]=(CONFIGIT)GetProcAddress(hinstLib[Slot], "ModuleConfig");
		PakPortWriteCalls[Slot]=(PACKPORTWRITE) GetProcAddress(hinstLib[Slot], "PackPortWrite");
		PakPortReadCalls[Slot]=(PACKPORTREAD) GetProcAddress(hinstLib[Slot], "PackPortRead");
		SetInteruptCallPointerCalls[Slot]=(SETINTERUPTCALLPOINTER)GetProcAddress(hinstLib[Slot], "AssertInterupt");

		DmaMemPointerCalls[Slot]=(DMAMEMPOINTERS) GetProcAddress(hinstLib[Slot], "MemPointers");
		SetCartCalls[Slot]=(SETCARTPOINTER) GetProcAddress(hinstLib[Slot], "SetCart"); //HERE

		HeartBeatCalls[Slot]=(HEARTBEAT) GetProcAddress(hinstLib[Slot], "HeartBeat");
		PakMemWrite8Calls[Slot]=(MEMWRITE8) GetProcAddress(hinstLib[Slot], "PakMemWrite8");
		PakMemRead8Calls[Slot]=(MEMREAD8) GetProcAddress(hinstLib[Slot], "PakMemRead8");
		ModuleStatusCalls[Slot]=(MODULESTATUS) GetProcAddress(hinstLib[Slot], "ModuleStatus");
		ModuleAudioSampleCalls[Slot]=(MODULEAUDIOSAMPLE) GetProcAddress(hinstLib[Slot], "ModuleAudioSample");
		ModuleResetCalls[Slot]=(MODULERESET) GetProcAddress(hinstLib[Slot], "ModuleReset");
		SetIniPathCalls[Slot]=(SETINIPATH) GetProcAddress(hinstLib[Slot], "SetIniPath");

		if (GetModuleNameCalls[Slot] == NULL)
		{
			UnloadModule(Slot);
			MessageBox(0,"Not a valid Module","Ok",0);
			return(0); //Error Not a Vcc Module
		}
		GetModuleNameCalls[Slot](ModuleNames[Slot],CatNumber[Slot],DynamicMenuCallbackCalls[Slot]); //Need to add address of local Dynamic menu callback function!
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

void UnloadModule(unsigned char Slot)
{
	GetModuleNameCalls[Slot]=NULL;
	ConfigModuleCalls[Slot]=NULL;
	PakPortWriteCalls[Slot]=NULL;
	PakPortReadCalls[Slot]=NULL;
	SetInteruptCallPointerCalls[Slot]=NULL;
	DmaMemPointerCalls[Slot]=NULL;
	HeartBeatCalls[Slot]=NULL;
	PakMemWrite8Calls[Slot]=NULL;
	PakMemRead8Calls[Slot]=NULL;
	ModuleStatusCalls[Slot]=NULL;
	ModuleAudioSampleCalls[Slot]=NULL;
	ModuleResetCalls[Slot]=NULL;
	SetIniPathCalls[Slot]=NULL;
//	SetCartCalls[Slot]=NULL;
	strcpy(ModulePaths[Slot],"");
	strcpy(ModuleNames[Slot],"Empty");
	strcpy(CatNumber[Slot],"");
	strcpy(SlotLabel[Slot],"Empty");
	if (hinstLib[Slot] !=NULL)
		FreeLibrary(hinstLib[Slot]);
	if (ExtRomPointers[Slot] !=NULL)
		free(ExtRomPointers[Slot]);
	hinstLib[Slot]=NULL;
	ExtRomPointers[Slot]=NULL;
	CartForSlot[Slot]=0;
	MenuCount[Slot]=0;
	return;
}

void LoadCartDLL(unsigned char Slot,char *DllPath)
{
	OPENFILENAME ofn ;
	unsigned char RetVal=0;

	UnloadModule(Slot);
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = NULL;
	ofn.lpstrFilter       = "Cartridges\0*.ROM;*.ccc;*.DLL\0\0"; // filter string
	ofn.nFilterIndex      = 1;                        // current filter index
	ofn.lpstrFile         = DllPath;                  // contains full path on return
	ofn.nMaxFile          = MAX_PATH;                 // sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;                     // filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH;                 // sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = MPIPath;                  // initial directory
	ofn.lpstrTitle        = TEXT("Choose Cartridge"); // title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	if ( GetOpenFileName (&ofn) ) {
		MountModule(Slot,DllPath);
		string tmp = ofn.lpstrFile;
		int idx;
		idx = tmp.find_last_of("\\");
		tmp = tmp.substr(0, idx);
		strcpy(MPIPath, tmp.c_str());
	}
	return;
}

void LoadConfig(void)
{
	// Get the module name from this DLL (MPI)
	char ModName[MAX_LOADSTRING]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);

	//PersistPaks=GetPrivateProfileInt(ModName, "PersistPaks", 1, IniFile);
	//DisableSCS=GetPrivateProfileInt(ModName,"DisableSCS", 0, IniFile);

	// Get default paths for modules
	GetPrivateProfileString("DefaultPaths", "MPIPath", "", MPIPath, MAX_PATH, IniFile);

	// Get the startup slot and set Chip select and SCS slots from ini file
	SwitchSlot=GetPrivateProfileInt(ModName,"SWPOSITION",3,IniFile);
	ChipSelectSlot=SwitchSlot;
	SpareSelectSlot=SwitchSlot;

	// Get saved path names for modules loaded in slots from ini file
	GetPrivateProfileString(ModName,"SLOT1","",ModulePaths[0],MAX_PATH,IniFile);
	CheckPath(ModulePaths[0]);
	GetPrivateProfileString(ModName,"SLOT2","",ModulePaths[1],MAX_PATH,IniFile);
	CheckPath(ModulePaths[1]);
	GetPrivateProfileString(ModName,"SLOT3","",ModulePaths[2],MAX_PATH,IniFile);
	CheckPath(ModulePaths[2]);
	GetPrivateProfileString(ModName,"SLOT4","",ModulePaths[3],MAX_PATH,IniFile);
	CheckPath(ModulePaths[3]);

	// Mount them
	for (int slot=0;slot<MAXPAX;slot++)
		if (*ModulePaths[slot] != '\0')
			MountModule(slot,ModulePaths[slot]);

	// Build the dynamic menu
	BuildDynaMenu();
	return;
}

void WriteConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	if (strcmp(MPIPath, "") != 0) {
		WritePrivateProfileString("DefaultPaths", "MPIPath", MPIPath, IniFile);
	}
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	WritePrivateProfileInt(ModName,"SWPOSITION",SwitchSlot,IniFile);
//	WritePrivateProfileInt(ModName, "PesistPaks", PersistPaks, IniFile);
//	WritePrivateProfileInt(ModName, "DisableSCS", DisableSCS, IniFile);
	ValidatePath(ModulePaths[0]);
	WritePrivateProfileString(ModName,"SLOT1",ModulePaths[0],IniFile);
	ValidatePath(ModulePaths[1]);
	WritePrivateProfileString(ModName,"SLOT2",ModulePaths[1],IniFile);
	ValidatePath(ModulePaths[2]);
	WritePrivateProfileString(ModName,"SLOT3",ModulePaths[2],IniFile);
	ValidatePath(ModulePaths[3]);
	WritePrivateProfileString(ModName,"SLOT4",ModulePaths[3],IniFile);

	return;
}

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
	return;
}

// The e_magic field in the MS-DOS header) of a PE file contains the ASCII
// characters "MZ". If file is a PE file then assume it is a DLL.
// This could break in some future windows version.
int FileID(const char *Filename)
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

void BuildDynaMenu(void)
{
	// DynamicMenuCallback() resides in VCC pakinterface. Make sure we have it's address
	if (DynamicMenuCallback == NULL) {
		MessageBox(0,"MPI internal menu error","Ok",0);
		return;
	}

	// Init the dynamic menus, then add a header line and then MPI config menu item
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",6000,0);

	// MenuID is an unsigned char 0 thru 99; 0 is used for reset and 1 for Config
	// The MMI DLL uses 2 - 19 for itself and allocates 20 for each slot's DLL.
	DynamicMenuCallback("MPI Config",5019,STANDALONE);

	// Build the rest of the menu for slots 4 thru 1
	for (int ndx=0;ndx<MenuCount[3];ndx++)
		DynamicMenuCallback(MenuName3[ndx],MenuId3[ndx]+80,Type3[ndx]);

	for (int ndx=0;ndx<MenuCount[2];ndx++)
		DynamicMenuCallback(MenuName2[ndx],MenuId2[ndx]+60,Type2[ndx]);

	for (int ndx=0;ndx<MenuCount[1];ndx++)
		DynamicMenuCallback(MenuName1[ndx],MenuId1[ndx]+40,Type1[ndx]);

	for (int ndx=0;ndx<MenuCount[0];ndx++)
		DynamicMenuCallback(MenuName0[ndx],MenuId0[ndx]+20,Type0[ndx]);

	DynamicMenuCallback( "",1,0);
}

// Callback for slot 1
void DynamicMenuCallback0( char *MenuName,int MenuId, int Type)
{
	PrintLogC("DynamicMenuCallback0 %4d %d '%s'\n",MenuId,Type,MenuName);
	if (MenuId==0)
	{
		MenuCount[0]=0;
		return;
	}

	if (MenuId==1)
	{
		BuildDynaMenu();
		return;
	}

	strcpy(MenuName0[MenuCount[0]],MenuName);
	MenuId0[MenuCount[0]]=MenuId;
	Type0[MenuCount[0]]=Type;
	MenuCount[0]++;
	return;
}

// Callback for Slot 2
void DynamicMenuCallback1( char *MenuName,int MenuId, int Type)
{
	PrintLogC("DynamicMenuCallback1 %4d %d '%s'\n",MenuId,Type,MenuName);
	if (MenuId==0)
	{
		MenuCount[1]=0;
		return;
	}

	if (MenuId==1)
	{
		BuildDynaMenu();
		return;
	}
	strcpy(MenuName1[MenuCount[1]],MenuName);
	MenuId1[MenuCount[1]]=MenuId;
	Type1[MenuCount[1]]=Type;
	MenuCount[1]++;
	return;
}

// Callback for Slot 3
void DynamicMenuCallback2( char *MenuName,int MenuId, int Type)
{
	PrintLogC("DynamicMenuCallback2 %4d %d '%s'\n",MenuId,Type,MenuName);
	if (MenuId==0)
	{
		MenuCount[2]=0;
		return;
	}

	if (MenuId==1)
	{
		BuildDynaMenu();
		return;
	}
	strcpy(MenuName2[MenuCount[2]],MenuName);
	MenuId2[MenuCount[2]]=MenuId;
	Type2[MenuCount[2]]=Type;
	MenuCount[2]++;
	return;
}

// Callback for Slot 4
void DynamicMenuCallback3( char *MenuName,int MenuId, int Type)
{
	PrintLogC("DynamicMenuCallback3 %4d %d '%s'\n",MenuId,Type,MenuName);
	if (MenuId==0)
	{
		MenuCount[3]=0;
		return;
	}

	if (MenuId==1)
	{
		BuildDynaMenu();
		return;
	}
	strcpy(MenuName3[MenuCount[3]],MenuName);
	MenuId3[MenuCount[3]]=MenuId;
	Type3[MenuCount[3]]=Type;
	MenuCount[3]++;
	return;
}

