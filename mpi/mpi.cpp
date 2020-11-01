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
#include "..\fileops.h"

#define MAXPAX 4
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short)=NULL;
static void (*MemWrite8)(unsigned char,unsigned short)=NULL;

static void (*PakSetCart)(unsigned char)=NULL;
static HINSTANCE g_hinstDLL=NULL;
static char ModuleNames[MAXPAX][MAX_LOADSTRING]={"Empty","Empty","Empty","Empty"};	
static char CatNumber[MAXPAX][MAX_LOADSTRING]={"","","",""};
static char SlotLabel[MAXPAX][MAX_LOADSTRING*2]={"Empty","Empty","Empty","Empty"};
//static 
static char ModulePaths[MAXPAX][MAX_PATH]={"","","",""};
static unsigned char *ExtRomPointers[MAXPAX]={NULL,NULL,NULL,NULL};
static unsigned int BankedCartOffset[MAXPAX]={0,0,0,0};
static unsigned char Temp,Temp2;
static char IniFile[MAX_PATH]="";
static char MPIPath[MAX_PATH];

using namespace std;

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
static void (*SetInteruptCallPointerCalls[MAXPAX]) ( ASSERTINTERUPT)={NULL,NULL,NULL,NULL};
static void (*DmaMemPointerCalls[MAXPAX]) (MEMREAD8,MEMWRITE8)={NULL,NULL,NULL,NULL};
//MenuName,int MenuId, int Type)

static char MenuName0[64][512],MenuName1[64][512],MenuName2[64][512],MenuName3[64][512];
static int MenuId0[64],MenuId1[64],MenuId2[64],MenuId3[64];
static int Type0[64],Type1[64],Type2[64],Type3[64];
//static int MenuIndex0=0,MenuIndex1=0,MenuIndex2=0,MenuIndex3=0;

static int MenuIndex[4]={0,0,0,0};


void SetCartSlot0(unsigned char);
void SetCartSlot1(unsigned char);
void SetCartSlot2(unsigned char);
void SetCartSlot3(unsigned char);
void BuildDynaMenu(void);
void DynamicMenuCallback0(char *,int, int);
void DynamicMenuCallback1(char *,int, int);
void DynamicMenuCallback2(char *,int, int);
void DynamicMenuCallback3(char *,int, int);
static unsigned char CartForSlot[MAXPAX]={0,0,0,0};
static void (*SetCarts[MAXPAX])(unsigned char)={SetCartSlot0,SetCartSlot1,SetCartSlot2,SetCartSlot3};
static void (*DynamicMenuCallbackCalls[MAXPAX])(char *,int, int)={DynamicMenuCallback0,DynamicMenuCallback1,DynamicMenuCallback2,DynamicMenuCallback3};
static void (*SetCartCalls[MAXPAX])(SETCART)={NULL,NULL,NULL,NULL};

static void (*SetIniPathCalls[MAXPAX]) (char *)={NULL,NULL,NULL,NULL};
static void (*DynamicMenuCallback)( char *,int, int)=NULL;
//***************************************************************
static HINSTANCE hinstLib[4]={NULL,NULL,NULL,NULL};
static unsigned char ChipSelectSlot=3,SpareSelectSlot=3,SwitchSlot=3,SlotRegister=255;

//Function Prototypes for this module
LRESULT CALLBACK Config(HWND,UINT,WPARAM,LPARAM);

unsigned char MountModule(unsigned char,char *);
void UnloadModule(unsigned char);
void LoadCartDLL(unsigned char,char *);
void LoadConfig(void);
void WriteConfig(void);
void ReadModuleParms(unsigned char,char *);
int FileID(char *);

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (fdwReason == DLL_PROCESS_DETACH ) //Clean Up 
	{
		WriteConfig();
		for(Temp=0;Temp<4;Temp++)
			UnloadModule(Temp);
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
	__declspec(dllexport) void ModuleConfig(unsigned char MenuID)
	{
		if ((MenuID >=10) & (MenuID <=19)) //Local config menus
		switch (MenuID)
		{
			case 10:
				LoadCartDLL(3,ModulePaths[3]);
				WriteConfig();
				BuildDynaMenu();
				break;

			case 11:
				UnloadModule(3);
				BuildDynaMenu();
				break;
			case 12:
				LoadCartDLL(2,ModulePaths[2]);
				WriteConfig();
				BuildDynaMenu();
				break;
			case 13:
				UnloadModule(2);
				BuildDynaMenu();
				break;
			case 14:
				LoadCartDLL(1,ModulePaths[1]);
				WriteConfig();
				BuildDynaMenu();
				break;
			case 15:
				UnloadModule(1);
				BuildDynaMenu();
				break;
			case 16:
				LoadCartDLL(0,ModulePaths[0]);
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
		if ( (MenuID>=20) & (MenuID <=40) )
			ConfigModuleCalls[0](MenuID-20);

		if ( (MenuID>40) & (MenuID <=60) )
			ConfigModuleCalls[1](MenuID-40);

		if ( (MenuID>61) & (MenuID <=80) )
			ConfigModuleCalls[2](MenuID-60);

		if ( (MenuID>80) & (MenuID <=100) )
			ConfigModuleCalls[3](MenuID-80);
		return;
	}

}

// This captures the Function transfer point for the CPU assert interupt 
extern "C" 
{
	__declspec(dllexport) void AssertInterupt(ASSERTINTERUPT Dummy)
	{
		AssertInt=Dummy;
		for (Temp=0;Temp<4;Temp++)
		{
			if(SetInteruptCallPointerCalls[Temp] !=NULL)
				SetInteruptCallPointerCalls[Temp](AssertInt);
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
			SpareSelectSlot= (Data & 3);
			ChipSelectSlot= ( (Data & 0x30)>>4);
			SlotRegister=Data;
			PakSetCart(0);
			if (CartForSlot[SpareSelectSlot]==1)
				PakSetCart(1);
			return;
		}
//		if ( (Port>=0x40) & (Port<=0x5F))
//		{
//			BankedCartOffset[SpareSelectSlot]=(Data & 15)<<14;
//			if ( PakPortWriteCalls[SpareSelectSlot] != NULL)
//				PakPortWriteCalls[SpareSelectSlot](Port,Data);
//		}
//		else
			for(unsigned char Temp=0;Temp<4;Temp++)
				if (PakPortWriteCalls[Temp] != NULL)
					PakPortWriteCalls[Temp](Port,Data);
		return;
	}
}

extern "C"
{
	__declspec(dllexport) unsigned char PackPortRead(unsigned char Port)
	{
		if (Port == 0x7F)
		{	
			SlotRegister&=0xCC;
			SlotRegister|=(SpareSelectSlot | (ChipSelectSlot<<4));
			return(SlotRegister);
		}

//		if ( (Port>=0x40) & (Port<=0x5F))
//		{
//			if ( PakPortReadCalls[SpareSelectSlot] != NULL)
//				return(PakPortReadCalls[SpareSelectSlot](Port));
//			else
//				return(NULL);
//		}
		Temp2=0;
		for (Temp=0;Temp<4;Temp++)
		{
			if ( PakPortReadCalls[Temp] !=NULL)
			{
				Temp2=PakPortReadCalls[Temp](Port); //Find a Module that return a value 
				if (Temp2!= 0)
					return(Temp2);
			}
		}
		return(0);
	}
}

extern "C"
{
	__declspec(dllexport) void HeartBeat(void)
	{
		for (Temp=0;Temp<4;Temp++)
			if (HeartBeatCalls[Temp] != NULL)
				HeartBeatCalls[Temp]();
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
		sprintf(MyStatus,"MPI:%i,%i",ChipSelectSlot,SpareSelectSlot);
		for (Temp=0;Temp<4;Temp++)
		{
			strcpy(TempStatus,"");
			if (ModuleStatusCalls[Temp] != NULL)
			{
				ModuleStatusCalls[Temp](TempStatus);
				strcat(MyStatus,"|");
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
		for (Temp=0;Temp<4;Temp++)
			if (ModuleAudioSampleCalls[Temp] != NULL)
				TempSample+=ModuleAudioSampleCalls[Temp]();
			
		return(TempSample) ;
	}
}

extern "C" 
{
	__declspec(dllexport) unsigned char ModuleReset (void)
	{
		ChipSelectSlot=SwitchSlot;	
		SpareSelectSlot=SwitchSlot;	
		for (Temp=0;Temp<4;Temp++)
		{
			BankedCartOffset[Temp]=0; //Do I need to keep independant selects?
			
			if (ModuleResetCalls[Temp] !=NULL)
				ModuleResetCalls[Temp]();
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


void CPUAssertInterupt(unsigned char Interupt,unsigned char Latencey)
{
	AssertInt(Interupt,Latencey);
	return;
}

extern "C"
{
	__declspec(dllexport) void SetCart(SETCART Pointer)
	{
		
		PakSetCart=Pointer;
		return;
	}
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned short EDITBOXS[4]={IDC_EDIT1,IDC_EDIT2,IDC_EDIT3,IDC_EDIT4};
	unsigned short INSERTBTN[4]={ID_INSERT1,ID_INSERT2,ID_INSERT3,ID_INSERT4};
	unsigned short REMOVEBTN[4]={ID_REMOVE1,ID_REMOVE2,ID_REMOVE3,ID_REMOVE4};
	unsigned short CONFIGBTN[4]={ID_CONFIG1,ID_CONFIG2,ID_CONFIG3,ID_CONFIG4};
	char ConfigText[1024]="";

	unsigned char Temp=0;
	switch (message)
	{
		case WM_INITDIALOG:
			for (Temp=0;Temp<4;Temp++)
				SendDlgItemMessage(hDlg,EDITBOXS[Temp],WM_SETTEXT,5,(LPARAM)(LPCSTR) SlotLabel[Temp] );
			SendDlgItemMessage(hDlg,IDC_PAKSELECT,TBM_SETRANGE,TRUE,MAKELONG(0,3) );
			SendDlgItemMessage(hDlg,IDC_PAKSELECT,TBM_SETPOS,TRUE,SwitchSlot);
			ReadModuleParms(SwitchSlot,ConfigText);
			SendDlgItemMessage(hDlg,IDC_MODINFO,WM_SETTEXT,strlen(ConfigText),(LPARAM)(LPCSTR)ConfigText );

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

			for (Temp=0;Temp<4;Temp++)
			{
				if ( LOWORD(wParam) == INSERTBTN[Temp] )
				{
					LoadCartDLL(Temp,ModulePaths[Temp]);
					for (Temp=0;Temp<4;Temp++)
						SendDlgItemMessage(hDlg,EDITBOXS[Temp],WM_SETTEXT,strlen(SlotLabel[Temp]),(LPARAM)(LPCSTR)SlotLabel[Temp] );
				}
			}

			for (Temp=0;Temp<4;Temp++)
			{
				if ( LOWORD(wParam) == REMOVEBTN[Temp] )
				{
					UnloadModule(Temp);	
					SendDlgItemMessage(hDlg,EDITBOXS[Temp],WM_SETTEXT,strlen(SlotLabel[Temp]),(LPARAM)(LPCSTR)SlotLabel[Temp] );
				}
			}

			for (Temp=0;Temp<4;Temp++)
			{
				if ( LOWORD(wParam) == CONFIGBTN[Temp] )
				{
					if (ConfigModuleCalls[Temp] != NULL)
						ConfigModuleCalls[Temp](NULL);
				}
			}
			return TRUE;
		break;

		case WM_HSCROLL:
			SwitchSlot=(unsigned char) SendDlgItemMessage(hDlg,IDC_PAKSELECT,TBM_GETPOS,(WPARAM) 0, (WPARAM) 0);
			SpareSelectSlot= SwitchSlot;
			ChipSelectSlot= SwitchSlot;
			ReadModuleParms(SwitchSlot,ConfigText);
			SendDlgItemMessage(hDlg,IDC_MODINFO,WM_SETTEXT,strlen(ConfigText),(LPARAM)(LPCSTR)ConfigText );
			PakSetCart(0);
			if (CartForSlot[SpareSelectSlot]==1)
				PakSetCart(1);
		break;
	}
    return FALSE;
}


unsigned char MountModule(unsigned char Slot,char *ModName)
{
	unsigned char ModuleType=0;
	char ModuleName[MAX_PATH]="";
	unsigned int index=0;
	strcpy(ModuleName,ModName);
	FILE *rom_handle;
	if (Slot>3)
		return(0);
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
	MenuIndex[Slot]=0;
	return;
}

void LoadCartDLL(unsigned char Slot,char *DllPath)
{
	OPENFILENAME ofn ;
	unsigned char RetVal=0;

	UnloadModule(Slot);
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner		  = NULL;
	ofn.lpstrFilter = "Program Packs\0*.ROM;*.ccc;*.DLL\0\0";			// filter string
	ofn.nFilterIndex      = 1 ;								// current filter index
	ofn.lpstrFile         = DllPath;						// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;						// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;							// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH ;						// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = MPIPath;							// initial directory
	ofn.lpstrTitle        = TEXT("Load Program Pack");	// title bar string
	ofn.Flags             = OFN_HIDEREADONLY;
	if ( GetOpenFileName (&ofn) )
	{
		RetVal= MountModule( Slot,DllPath);
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
	char ModName[MAX_LOADSTRING]="";
	LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	GetPrivateProfileString("DefaultPaths", "MPIPath", "", MPIPath, MAX_PATH, IniFile);
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
	for (Temp=0;Temp<4;Temp++)
		if (strlen(ModulePaths[Temp]) !=0)
			MountModule(Temp,ModulePaths[Temp]);
	BuildDynaMenu();
	return;
}

void WriteConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	if (MPIPath != "") { WritePrivateProfileString("DefaultPaths", "MPIPath", MPIPath, IniFile); }
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
	return;
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
	return;
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
	return;
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
	return;
}