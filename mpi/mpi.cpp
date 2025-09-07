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
#include "../DialogOps.h"
#include "../MachineDefs.h"
#include "../logger.h"

// Number of slots supported. Changing this might require code modification
#define NUMSLOTS 4

// Is a port a disk port?
#define ISDISKPORT(p) ((p > 0x3F) && (p < 0x60))

using namespace std;
static void (*AssertInt)(unsigned char, unsigned char)=nullptr;
static unsigned char (*MemRead8)(unsigned short)=nullptr;
static void (*MemWrite8)(unsigned char,unsigned short)=nullptr;

static void (*PakSetCart)(unsigned char)=nullptr;
static HINSTANCE g_hinstDLL=nullptr;
static char CatNumber[NUMSLOTS][MAX_LOADSTRING]={"","","",""};
static char SlotLabel[NUMSLOTS][MAX_LOADSTRING*2]={"Empty","Empty","Empty","Empty"};
//static unsigned char PersistPaks = 0;
//static unsigned char DisableSCS = 0;
static char ModulePaths[NUMSLOTS][MAX_PATH]={"","","",""};
static char ModuleNames[NUMSLOTS][MAX_LOADSTRING]={"Empty","Empty","Empty","Empty"};
static unsigned char *ExtRomPointers[NUMSLOTS]={nullptr,nullptr,nullptr,nullptr};
static unsigned int BankedCartOffset[NUMSLOTS]={0,0,0,0};
static char IniFile[MAX_PATH]="";
static char MPIPath[MAX_PATH];

//**************************************************************
//Array of fuction pointer for each Slot
static void (*GetModuleNameCalls[NUMSLOTS])(char *,char *,DYNAMICMENUCALLBACK)={nullptr,nullptr,nullptr,nullptr};
static void (*ConfigModuleCalls[NUMSLOTS])(unsigned char)={nullptr,nullptr,nullptr,nullptr};
static void (*HeartBeatCalls[NUMSLOTS])(void)={nullptr,nullptr,nullptr,nullptr};
static void (*PakPortWriteCalls[NUMSLOTS])(unsigned char,unsigned char)={nullptr,nullptr,nullptr,nullptr};
static unsigned char (*PakPortReadCalls[NUMSLOTS])(unsigned char)={nullptr,nullptr,nullptr,nullptr};
static void (*PakMemWrite8Calls[NUMSLOTS])(unsigned char,unsigned short)={nullptr,nullptr,nullptr,nullptr};
static unsigned char (*PakMemRead8Calls[NUMSLOTS])(unsigned short)={nullptr,nullptr,nullptr,nullptr};
static void (*ModuleStatusCalls[NUMSLOTS])(char *)={nullptr,nullptr,nullptr,nullptr};
static unsigned short (*ModuleAudioSampleCalls[NUMSLOTS])(void)={nullptr,nullptr,nullptr,nullptr};
static void (*ModuleResetCalls[NUMSLOTS]) (void)={nullptr,nullptr,nullptr,nullptr};
//Set callbacks for the DLL to call
static void (*SetInteruptCallPointerCalls[NUMSLOTS]) ( PAKINTERUPT)={nullptr,nullptr,nullptr,nullptr};
static void (*DmaMemPointerCalls[NUMSLOTS]) (MEMREAD8,MEMWRITE8)={nullptr,nullptr,nullptr,nullptr};
//MenuName,int MenuId, int Type)
static char MenuName0[64][512],MenuName1[64][512],MenuName2[64][512],MenuName3[64][512];
static int MenuId0[64],MenuId1[64],MenuId2[64],MenuId3[64];
static int Type0[64],Type1[64],Type2[64],Type3[64];
static int MenuCount[NUMSLOTS]={0};

static unsigned short EDITBOXS[4]={IDC_EDIT1,IDC_EDIT2,IDC_EDIT3,IDC_EDIT4};
static unsigned short RADIOBTN[4]={IDC_SELECT1,IDC_SELECT2,IDC_SELECT3,IDC_SELECT4};
static unsigned short INSBOXS[4]={IDC_INSERT1,IDC_INSERT2,IDC_INSERT3,IDC_INSERT4};

void UpdateSlotContent(int);
void UpdateSlotConfig(int);
void UpdateSlotSelect(int);
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
static unsigned char CartForSlot[NUMSLOTS]={0};
static void (*SetCarts[NUMSLOTS])(unsigned char)={SetCartSlot0,SetCartSlot1,SetCartSlot2,SetCartSlot3};
static void (*DynamicMenuCallbackCalls[NUMSLOTS])(char *,int, int)={DynamicMenuCallback0,DynamicMenuCallback1,DynamicMenuCallback2,DynamicMenuCallback3};
static void (*SetCartCalls[NUMSLOTS])(SETCART)={nullptr};

static void (*SetIniPathCalls[NUMSLOTS]) (char *)={nullptr};
static void (*DynamicMenuCallback)( char *,int, int)=nullptr;
//***************************************************************
static HINSTANCE hinstLib[NUMSLOTS]={nullptr};
static unsigned char ChipSelectSlot=3,SpareSelectSlot=3,SwitchSlot=3,SlotRegister=255;
static HWND hConfDlg=nullptr;
static HWND hParentWindow=nullptr;

//Function Prototypes for this module
LRESULT CALLBACK MpiConfigDlg(HWND,UINT,WPARAM,LPARAM);

unsigned char MountModule(unsigned char,const char *);
void UnloadModule(unsigned char);
void UpdateCartDLL(unsigned char,char *);
void LoadConfig(void);
void WriteConfig(void);
void ReadModuleParms(unsigned char,char *);
int FileID(const char *);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD  Reason, LPVOID Reserved)
{
	switch (Reason) {
	case DLL_PROCESS_ATTACH:
		//PrintLogC("MPI process attach %d\n",hinstDLL);
		g_hinstDLL = hinstDLL;
		break;
	case DLL_PROCESS_DETACH:
		// Close dialog before unloading modules so config is saved
		CloseCartDialog(hConfDlg);  // defined in DialogOps
		for (int slot=0;slot<NUMSLOTS;slot++) UnloadModule(slot);
		break;
	}
	return TRUE;
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
		// Vcc subtracts 5000 from the wmId to get MenuID
		// MPI config wmId is 5019 so 19 is used here.
		if (MenuID == 19) {
			if (!hConfDlg)
				hConfDlg = CreateDialog(g_hinstDLL, (LPCTSTR)IDD_DIALOG1,
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
		for (int slot=0;slot<NUMSLOTS;slot++) {
			if(SetInteruptCallPointerCalls[slot] !=nullptr)
				SetInteruptCallPointerCalls[slot](AssertInt);
		}
		return;
	}
}

extern "C"
{
	__declspec(dllexport) void PackPortWrite(unsigned char Port,unsigned char Data)
	{
		if (Port == 0x7F) //Multi-Pak selects
		{
			SpareSelectSlot= (Data & 3);          //SCS
			ChipSelectSlot= ( (Data & 0x30)>>4);  //CTS
			SlotRegister=Data;
			PakSetCart(0);
			if (CartForSlot[SpareSelectSlot]==1)
				PakSetCart(1);
			return;
		}

		// Only write disk ports (0x40-0x5F) if SCS is set
		if (ISDISKPORT(Port)) {
			BankedCartOffset[SpareSelectSlot]=(Data & 15)<<14;
			if ( PakPortWriteCalls[SpareSelectSlot] != nullptr)
				PakPortWriteCalls[SpareSelectSlot](Port,Data);
		} else {
			for (int slot=0;slot<NUMSLOTS;slot++)
				if (PakPortWriteCalls[slot] != nullptr)
					PakPortWriteCalls[slot](Port,Data);
		}
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

		// Only read disk ports (0x40-0x5F) if SCS is set
		if (ISDISKPORT(Port)) {
			if ( PakPortReadCalls[SpareSelectSlot] != nullptr)
				return(PakPortReadCalls[SpareSelectSlot](Port));
			else
				return(0);
		}

		for (int slot=0;slot<NUMSLOTS;slot++) {
			if ( PakPortReadCalls[slot] !=nullptr) {
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
		for (int slot=0;slot<NUMSLOTS;slot++)
			if (HeartBeatCalls[slot] != nullptr)
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
		if (ExtRomPointers[ChipSelectSlot] != nullptr)
			return(ExtRomPointers[ChipSelectSlot][(Address & 32767)+BankedCartOffset[ChipSelectSlot]]); //Bank Select ???
		if (PakMemRead8Calls[ChipSelectSlot] != nullptr)
			return(PakMemRead8Calls[ChipSelectSlot](Address));

		return 0;
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
		for (int slot=0;slot<NUMSLOTS;slot++)
		{
			strcpy(TempStatus,"");
			if (ModuleStatusCalls[slot] != nullptr)
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
		for (int slot=0;slot<NUMSLOTS;slot++)
			if (ModuleAudioSampleCalls[slot] != nullptr)
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
		for (int slot=0;slot<NUMSLOTS;slot++)
		{
			BankedCartOffset[slot]=0; //Do I need to keep independant selects?

			if (ModuleResetCalls[slot] != nullptr)
				ModuleResetCalls[slot]();
		}
		if (PakSetCart != nullptr) {
			PakSetCart(0);
			if (CartForSlot[SpareSelectSlot]==1)
				PakSetCart(1);
		}
		return 0;
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
	SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

LRESULT CALLBACK MpiConfigDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message) {
	case WM_CLOSE:
		WriteConfig();
		DestroyWindow(hDlg);
		hConfDlg=nullptr;
		return TRUE;
		break;
	case WM_INITDIALOG:
		hParentWindow = GetParent(hDlg);
		CenterDialog(hDlg);
		for (int Slot=0;Slot<NUMSLOTS;Slot++) {
			SendDlgItemMessage(hDlg,EDITBOXS[Slot],WM_SETTEXT,0,(LPARAM)SlotLabel[Slot]);
			if ((strcmp(ModuleNames[Slot],"Empty") != 0) || hinstLib[Slot])
				SendDlgItemMessage(hDlg,INSBOXS[Slot],WM_SETTEXT,0,(LPARAM)"X");
			else
				SendDlgItemMessage(hDlg,INSBOXS[Slot],WM_SETTEXT,0,(LPARAM)">");
		}
		UpdateSlotSelect(SwitchSlot);
		return TRUE;
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SELECT1:
			UpdateSlotSelect(0);
			return TRUE;
		case IDC_SELECT2:
			UpdateSlotSelect(1);
			return TRUE;
		case IDC_SELECT3:
			UpdateSlotSelect(2);
			return TRUE;
		case IDC_SELECT4:
			UpdateSlotSelect(3);
			return TRUE;
		case IDC_INSERT1:
			UpdateSlotContent(0);
			return TRUE;
		case IDC_INSERT2:
			UpdateSlotContent(1);
			return TRUE;
		case IDC_INSERT3:
			UpdateSlotContent(2);
			return TRUE;
		case IDC_INSERT4:
			UpdateSlotContent(3);
			return TRUE;
		case ID_CONFIG1:
			UpdateSlotConfig(0);
			return TRUE;
		case ID_CONFIG2:
			UpdateSlotConfig(1);
			return TRUE;
		case ID_CONFIG3:
			UpdateSlotConfig(2);
			return TRUE;
		case ID_CONFIG4:
			UpdateSlotConfig(3);
			return TRUE;
			break;
		} // End switch LOWORD
		break;
	} // End switch message
	return FALSE;
}

void UpdateSlotSelect(int slot)
{
	char ConfigText[1024]="";
	ReadModuleParms(slot,ConfigText);
	SendDlgItemMessage(hConfDlg,IDC_MODINFO,WM_SETTEXT,0,(LPARAM)(LPCSTR)ConfigText);

	for (int ndx=0;ndx<4;ndx++) {
		if (ndx==slot) {
			SendDlgItemMessage(hConfDlg, RADIOBTN[ndx], BM_SETCHECK, 1, 0);
		} else {
			SendDlgItemMessage(hConfDlg, RADIOBTN[ndx], BM_SETCHECK, 0, 0);
		}
	}
	SwitchSlot = slot;
	SpareSelectSlot = slot;
	ChipSelectSlot = slot;
	if (CartForSlot[slot]==1)
		PakSetCart(1);
	else
		PakSetCart(0);
}

void UpdateSlotContent(int Slot)
{
	// Disable Slot changes if parent is disabled.  This prevents user using the
	// config dialog to eject a cartridge while VCC main is using a modal dialog
	// Otherwise user can crash VCC by unloading a disk cart while inserting a disk
	if (!IsWindowEnabled(hParentWindow)) {
		MessageBox(hConfDlg,"Cannot change slot content with dialog open","ERROR",MB_ICONERROR);
		return;
	}

	UpdateCartDLL(Slot,ModulePaths[Slot]);
	SendDlgItemMessage(hConfDlg,EDITBOXS[Slot],WM_SETTEXT,0,(LPARAM)SlotLabel[Slot]);
	if ((strcmp(ModuleNames[Slot],"Empty") != 0) || hinstLib[Slot])
		SendDlgItemMessage(hConfDlg,INSBOXS[Slot],WM_SETTEXT,0,(LPARAM)"X");
	else
		SendDlgItemMessage(hConfDlg,INSBOXS[Slot],WM_SETTEXT,0,(LPARAM)">");
	BuildDynaMenu();
	return;
}

void UpdateSlotConfig(int slot)
{
	if (ConfigModuleCalls[slot] != nullptr)
		ConfigModuleCalls[slot](0);
}

unsigned char MountModule(unsigned char Slot,const char *ModuleName)
{
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
		if (ExtRomPointers[Slot]==nullptr)
		{
			MessageBox(0,"Rom pointer is NULL","Error",0);
			return(0); //Can Allocate RAM
		}
		rom_handle=fopen(MountName,"rb");
		if (rom_handle==nullptr)
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
		if (hinstLib[Slot]==nullptr)
			return(0);	//Error Can't open File
		if (hinstLib[Slot] == g_hinstDLL) {
			MessageBox(hConfDlg,"Can not insert MPI into a slot","ERROR",MB_ICONERROR);
			UnloadModule(Slot);
			return(0);
		}
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

		if (GetModuleNameCalls[Slot] == nullptr)
		{
			UnloadModule(Slot);
			MessageBox(0,"Not a valid Module","Ok",0);
			return(0); //Error Not a Vcc Module
		}
		GetModuleNameCalls[Slot](ModuleNames[Slot],CatNumber[Slot],DynamicMenuCallbackCalls[Slot]); //Need to add address of local Dynamic menu callback function!
		strcpy(SlotLabel[Slot],ModuleNames[Slot]);
		strcat(SlotLabel[Slot],"  ");
		strcat(SlotLabel[Slot],CatNumber[Slot]);

		if (SetInteruptCallPointerCalls[Slot] !=nullptr)
			SetInteruptCallPointerCalls[Slot](AssertInt);
		if (DmaMemPointerCalls[Slot] !=nullptr)
			DmaMemPointerCalls[Slot](MemRead8,MemWrite8);
		if (SetIniPathCalls[Slot] != nullptr)
			SetIniPathCalls[Slot](IniFile);
		if (SetCartCalls[Slot] !=nullptr)
			SetCartCalls[Slot](*SetCarts[Slot]);	//Transfer the address of the SetCart routin to the pak
													//For the multpak there is 1 for each slot se we know where it came from
		if (ModuleResetCalls[Slot]!=nullptr)
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
	GetModuleNameCalls[Slot]=nullptr;
	ConfigModuleCalls[Slot]=nullptr;
	PakPortWriteCalls[Slot]=nullptr;
	PakPortReadCalls[Slot]=nullptr;
	SetInteruptCallPointerCalls[Slot]=nullptr;
	DmaMemPointerCalls[Slot]=nullptr;
	HeartBeatCalls[Slot]=nullptr;
	PakMemWrite8Calls[Slot]=nullptr;
	PakMemRead8Calls[Slot]=nullptr;
	ModuleStatusCalls[Slot]=nullptr;
	ModuleAudioSampleCalls[Slot]=nullptr;
	ModuleResetCalls[Slot]=nullptr;
	SetIniPathCalls[Slot]=nullptr;
//	SetCartCalls[Slot]=nullptr;
	strcpy(ModulePaths[Slot],"");
	strcpy(ModuleNames[Slot],"Empty");
	strcpy(CatNumber[Slot],"");
	strcpy(SlotLabel[Slot],"Empty");
	if (ExtRomPointers[Slot] !=nullptr)
		free(ExtRomPointers[Slot]);
	ExtRomPointers[Slot]=nullptr;
	CartForSlot[Slot]=0;
	MenuCount[Slot]=0;
	if (hinstLib[Slot] !=nullptr)
		FreeLibrary(hinstLib[Slot]);
	hinstLib[Slot]=nullptr;
	return;
}

void UpdateCartDLL(unsigned char Slot,char *DllPath)
{
	if ((strcmp(ModuleNames[Slot],"Empty") != 0) || hinstLib[Slot]) {
		UnloadModule(Slot);
	} else {
		FileDialog dlg;
		dlg.setTitle("Load Program Pack");
		dlg.setInitialDir(MPIPath);
		dlg.setFilter("DLL Packs\0*.dll\0Rom Packs\0*.ROM;*.ccc;*.pak\0\0");
		dlg.setFlags(OFN_FILEMUSTEXIST);
		if (dlg.show(0,hConfDlg)) {
			MountModule(Slot,dlg.path());
			dlg.getdir(MPIPath);
		}
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
	for (int slot=0;slot<NUMSLOTS;slot++)
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

	if (ConfigModuleCalls[Slot]!=nullptr)
		strcat(String,"Has Configurable options\r\n");

	if (SetIniPathCalls[Slot]!=nullptr)
		strcat(String,"Saves Config Info\r\n");

	if (PakPortWriteCalls[Slot]!=nullptr)
		strcat(String,"Is IO writable\r\n");

	if (PakPortReadCalls[Slot]!=nullptr)
		strcat(String,"Is IO readable\r\n");

	if (SetInteruptCallPointerCalls[Slot]!=nullptr)
		strcat(String,"Generates Interupts\r\n");

	if (DmaMemPointerCalls[Slot]!=nullptr)
		strcat(String,"Generates DMA Requests\r\n");

	if (HeartBeatCalls[Slot]!=nullptr)
		strcat(String,"Needs Heartbeat\r\n");

	if (ModuleAudioSampleCalls[Slot]!=nullptr)
		strcat(String,"Analog Audio Outputs\r\n");

	if (PakMemWrite8Calls[Slot]!=nullptr)
		strcat(String,"Needs CS Write\r\n");

	if (PakMemRead8Calls[Slot]!=nullptr)
		strcat(String,"Needs CS Read (onboard ROM)\r\n");

	if (ModuleStatusCalls[Slot]!=nullptr)
		strcat(String,"Returns Status\r\n");

	if (ModuleResetCalls[Slot]!=nullptr)
		strcat(String,"Needs Reset Notification\r\n");

//	if (SetCartCalls[Slot]!=nullptr)
//		strcat(String,"Asserts CART\r\n");
	return;
}

// The e_magic field in the MS-DOS header) of a PE file contains the ASCII
// characters "MZ". If file is a PE file then assume it is a DLL.
// This could break in some future windows version.
int FileID(const char *Filename)
{
	FILE *DummyHandle=nullptr;
	char Temp[3]="";
	DummyHandle=fopen(Filename,"rb");
	if (DummyHandle==nullptr)
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

// This gets called on mpi startup and each time a module is inserted or deleted
void BuildDynaMenu(void)
{
	// DynamicMenuCallback() resides in VCC pakinterface. Make sure we have it's address
	if (DynamicMenuCallback == nullptr) {
		MessageBox(0,"MPI internal menu error","Ok",0);
		return;
	}

	// Init the dynamic menus, then add a header line and then MPI config menu item
	DynamicMenuCallback("",0,0);
	DynamicMenuCallback("",6000,0);

	// The second argument to DynamicMenuCallback establishes the wmID for mpithe config.
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

