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

#define DIRECTINPUT_VERSION 0x0800

#include "windows.h"
#include "defines.h"
//#include <iostream.h>
#include <commctrl.h>
#include "stdio.h"
#include <Richedit.h>
#include "resource.h"
#include "config.h"
#include "tcc1014graphics.h"
#include "mc6821.h"
#include "Vcc.h"
#include "tcc1014mmu.h"
#include "audio.h"
#include "pakinterface.h"
#include "vcc.h"
#include "DirectDrawInterface.h"
#include "joystickinput.h"
#include "keyboard.h"
#include "fileops.h"
#include "Cassette.h"

//#include "logger.h"

#define MAXCARDS 12
LRESULT CALLBACK CpuConfig(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK AudioConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MiscConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DisplayConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InputConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK JoyStickConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TapeConfig(HWND , UINT , WPARAM , LPARAM );
LRESULT CALLBACK BitBanger(HWND , UINT , WPARAM , LPARAM );
int SelectFile(char *);
static unsigned short int	Ramchoice[4]={IDC_128K,IDC_512K,IDC_2M,IDC_8M};
static unsigned short int	Cpuchoice[2]={IDC_6809,IDC_6309};
static unsigned short int	Monchoice[2]={IDC_COMPOSITE,IDC_RGB};
static HICON CpuIcons[2],MonIcons[2],JoystickIcons[4];
static unsigned char temp=0,temp2=0;
static char IniFileName[]="Vcc.ini";
static char IniFilePath[MAX_PATH]="";
static char TapeFileName[MAX_PATH]="";
static char ExecDirectory[MAX_PATH]="";
static char SerialCaptureFile[MAX_PATH]="";
static char TextMode=1,PrtMon=0;;
static unsigned char NumberofJoysticks=0;
char OutBuffer[MAX_PATH]="";
char AppName[MAX_LOADSTRING]="";
STRConfig CurrentConfig;
static STRConfig TempConfig;
extern SystemState EmuState;
extern char StickName[MAXSTICKS][STRLEN];
static KeyBoardTable RawTable [4][100];
static KeyBoardTable KbTemp[100];
KeyBoardTable Table[100];
JoyStick Left;
JoyStick Right;
static JoyStick TempLeft,TempRight;
static unsigned int TapeCounter=0;
static unsigned char Tmode=STOP;
char Tmodes[4][10]={"STOP","PLAY","REC","STOP"};

static int NumberOfSoundCards=0;
unsigned char XY2Disp (unsigned char,unsigned char);
void Disp2XY(unsigned char *,unsigned char *,unsigned char);
void BuildKeyboardTable(void);
void SortKBtable(unsigned char);
void ReadKeymapFile(char *);
void WriteKeymapFile(char *);
void RefreshJoystickStatus(void);
static SndCardList SoundCards[MAXCARDS];
static HWND hDlgBar=NULL,hDlgTape=NULL;
unsigned char TranslateDisp2Scan[84];
unsigned char TranslateScan2Disp[84]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,32,38,20,33,35,40,36,24,30,31,42,43,55,52,16,34,19,21,22,23,25,26,27,45,46,0,51,44,41,39,18,37,17,29,28,47,48,49,51,0,53,54,50,66,67,0,0,0,0,0,0,0,0,0,0,58,64,60,0,62,0,63,0,59,65,61,56,57};
char Entries[68][8]={"","ESC","1","2","3","4","5","6","7","8","9","0","-","=","BackSp","Tab","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","[","]","Bkslash",";","'","Comma",".","/","CapsLk","Shift","Ctrl","Alt","Space","Enter","Insert","Delete","Home","End","PgUp","PgDown","Left","Right","Up","Down","F1","F2"};
//					  0	 1      2	3	4	5	6	7	8	9	10	11	12	13	14	    15    16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44        45  46  47      48  49   50       51     52     53    54      55      56       57       58     59    60     61       62     63      64   65     66   67	

char CCEntries[57][16]={"","@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","Up","Down","Left","Right","Space","0","1","2","3","4","5","6","7","8","9",":",";","Comma","-",".","/","Enter","Clear","Break","Alt","Ctrl","F1","F2","Shift"};


CHARFORMAT CounterText;
CHARFORMAT ModeText;

void LoadConfig(SystemState *LCState)
{
	HANDLE hr=NULL;
	LoadString(NULL, IDS_APP_TITLE,AppName, MAX_LOADSTRING);
	GetModuleFileName(NULL,ExecDirectory,MAX_PATH);
	PathRemoveFileSpec(ExecDirectory);
	strcpy(CurrentConfig.PathtoExe,ExecDirectory);
	strcpy(IniFilePath,ExecDirectory);
	strcat(IniFilePath,"\\");
	strcat(IniFilePath,IniFileName);
	BuildKeyboardTable();
	LCState->ScanLines=0;
	NumberOfSoundCards=GetSoundCardList(SoundCards);
	ReadIniFile();
	CurrentConfig.RebootNow=0;
	UpdateConfig();
	RefreshJoystickStatus();
	SoundInit(EmuState.WindowHandle,SoundCards[CurrentConfig.SndOutDev].Guid,CurrentConfig.AudioRate);
	hr=CreateFile(IniFilePath,NULL,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hr==INVALID_HANDLE_VALUE) //No Ini File go create it
		WriteIniFile();
	else
		CloseHandle(hr);
	return;
}

unsigned char WriteIniFile(void)
{
	char FullMapPath[MAX_PATH]="";
	GetCurrentModule(CurrentConfig.ModulePath);
	ValidatePath(CurrentConfig.ModulePath);
	ValidatePath(CurrentConfig.KeymapFile);
	ValidatePath(CurrentConfig.ExternalBasicImage);
	WritePrivateProfileString("Version","Release",AppName,IniFilePath);
	WritePrivateProfileInt("CPU","DoubleSpeedClock",CurrentConfig.CPUMultiplyer,IniFilePath);
	WritePrivateProfileInt("CPU","FrameSkip",CurrentConfig.FrameSkip,IniFilePath);
	WritePrivateProfileInt("CPU","Throttle",CurrentConfig.SpeedThrottle,IniFilePath);
	WritePrivateProfileInt("CPU","CpuType",CurrentConfig.CpuType,IniFilePath);
	WritePrivateProfileString("Audio","SndCard",CurrentConfig.SoundCardName,IniFilePath);
	WritePrivateProfileInt("Audio","Rate",CurrentConfig.AudioRate,IniFilePath);
	WritePrivateProfileInt("Video","MonitorType",CurrentConfig.MonitorType,IniFilePath);
	WritePrivateProfileInt("Video","ScanLines",CurrentConfig.ScanLines,IniFilePath);
	WritePrivateProfileInt("Video","AllowResize",CurrentConfig.Resize,IniFilePath);
	WritePrivateProfileInt("Video","ForceAspect",CurrentConfig.Aspect,IniFilePath);
	WritePrivateProfileInt("Memory","RamSize",CurrentConfig.RamSize,IniFilePath);
	WritePrivateProfileInt("Misc","AutoStart",CurrentConfig.AutoStart,IniFilePath);
	WritePrivateProfileInt("Misc","CartAutoStart",CurrentConfig.CartAutoStart,IniFilePath);
	WritePrivateProfileString("Memory","ExternalBasicImage",CurrentConfig.ExternalBasicImage,IniFilePath);
	WritePrivateProfileString("Module","OnBoot",CurrentConfig.ModulePath,IniFilePath);
	WritePrivateProfileString("Misc","KeyMap",CurrentConfig.KeymapFile,IniFilePath);
	WritePrivateProfileInt("Misc","KeyMapIndex",CurrentConfig.KeyMap,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","UseMouse",Left.UseMouse,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Left",Left.Left,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Right",Left.Right,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Up",Left.Up,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Down",Left.Down,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Fire1",Left.Fire1,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Fire2",Left.Fire2,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","DiDevice",Left.DiDevice,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","UseMouse",Right.UseMouse,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Left",Right.Left,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Right",Right.Right,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Up",Right.Up,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Down",Right.Down,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Fire1",Right.Fire1,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Fire2",Right.Fire2,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","DiDevice",Right.DiDevice,IniFilePath);
	strcpy(FullMapPath,ExecDirectory);	
	strcat(FullMapPath,CurrentConfig.KeymapFile);
	WriteKeymapFile(FullMapPath);
	return(0);
}

unsigned char ReadIniFile(void)
{
	HANDLE hr=NULL;
	unsigned char Index=0;
	//Loads the config structure from the hard disk
	CurrentConfig.CPUMultiplyer = GetPrivateProfileInt("CPU","DoubleSpeedClock",2,IniFilePath);
	CurrentConfig.FrameSkip = GetPrivateProfileInt("CPU","FrameSkip",1,IniFilePath);
	CurrentConfig.SpeedThrottle = GetPrivateProfileInt("CPU","Throttle",1,IniFilePath);
	CurrentConfig.CpuType = GetPrivateProfileInt("CPU","CpuType",0,IniFilePath);
	CurrentConfig.AudioRate = GetPrivateProfileInt("Audio","Rate",3,IniFilePath);
	GetPrivateProfileString("Audio","SndCard","",CurrentConfig.SoundCardName,63,IniFilePath);
	CurrentConfig.MonitorType = GetPrivateProfileInt("Video","MonitorType",1,IniFilePath);
	CurrentConfig.ScanLines = GetPrivateProfileInt("Video","ScanLines",0,IniFilePath);
	CurrentConfig.Resize = GetPrivateProfileInt("Video","AllowResize",0,IniFilePath);	
	CurrentConfig.Aspect = GetPrivateProfileInt("Video","ForceAspect",0,IniFilePath);
	CurrentConfig.AutoStart = GetPrivateProfileInt("Misc","AutoStart",1,IniFilePath);
	CurrentConfig.CartAutoStart = GetPrivateProfileInt("Misc","CartAutoStart",1,IniFilePath);
	CurrentConfig.RamSize = GetPrivateProfileInt("Memory","RamSize",1,IniFilePath);
	GetPrivateProfileString("Memory","ExternalBasicImage","",CurrentConfig.ExternalBasicImage,MAX_PATH,IniFilePath);
	GetPrivateProfileString("Module","OnBoot","",CurrentConfig.ModulePath,MAX_PATH,IniFilePath);
	GetPrivateProfileString("Misc","KeyMap","KeyMap.ini",CurrentConfig.KeymapFile,MAX_PATH,IniFilePath);
	CurrentConfig.KeyMap=GetPrivateProfileInt("Misc","KeyMapIndex",0,IniFilePath);
	if (CurrentConfig.KeyMap>3)
		CurrentConfig.KeyMap=0;	//Default to DECB Mapping
	SortKBtable(CurrentConfig.KeyMap); 
	CheckPath(CurrentConfig.ModulePath);
	CheckPath(CurrentConfig.ExternalBasicImage);
	Left.UseMouse=GetPrivateProfileInt("LeftJoyStick","UseMouse",1,IniFilePath);
	Left.Left=GetPrivateProfileInt("LeftJoyStick","Left",75,IniFilePath);
	Left.Right=GetPrivateProfileInt("LeftJoyStick","Right",77,IniFilePath);
	Left.Up=GetPrivateProfileInt("LeftJoyStick","Up",72,IniFilePath);
	Left.Down=GetPrivateProfileInt("LeftJoyStick","Down",80,IniFilePath);
	Left.Fire1=GetPrivateProfileInt("LeftJoyStick","Fire1",59,IniFilePath);
	Left.Fire2=GetPrivateProfileInt("LeftJoyStick","Fire2",60,IniFilePath);
	Left.DiDevice=GetPrivateProfileInt("LeftJoyStick","DiDevice",0,IniFilePath);

	Right.UseMouse=GetPrivateProfileInt("RightJoyStick","UseMouse",1,IniFilePath);
	Right.Left=GetPrivateProfileInt("RightJoyStick","Left",75,IniFilePath);
	Right.Right=GetPrivateProfileInt("RightJoyStick","Right",77,IniFilePath);
	Right.Up=GetPrivateProfileInt("RightJoyStick","Up",72,IniFilePath);
	Right.Down=GetPrivateProfileInt("RightJoyStick","Down",80,IniFilePath);
	Right.Fire1=GetPrivateProfileInt("RightJoyStick","Fire1",59,IniFilePath);
	Right.Fire2=GetPrivateProfileInt("RightJoyStick","Fire2",60,IniFilePath);
	Right.DiDevice=GetPrivateProfileInt("RightJoyStick","DiDevice",0,IniFilePath);

	for (Index=0;Index<NumberOfSoundCards;Index++)
		if (!strcmp(SoundCards[Index].CardName,CurrentConfig.SoundCardName))
			CurrentConfig.SndOutDev=Index;
	TempConfig=CurrentConfig;
	InsertModule (CurrentConfig.ModulePath);	//Should this be here?
	return(0);
}

char * BasicRomName(void)
{
	return(CurrentConfig.ExternalBasicImage); 
}


LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	#define TABS 8
	static char TabTitles[TABS][10]={"Audio","CPU","Display","Keyboard","Joysticks","Misc","Tape","BitBanger"};
	static unsigned char TabCount=0,SelectedTab=0;
	static HWND hWndTabDialog;
	static HWND g_hWndConfig[TABS];
	TCITEM Tabs;
	switch (message)
	{
		case WM_INITDIALOG:
			InitCommonControls();
			TempConfig=CurrentConfig;
			CpuIcons[0]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_MOTO);
			CpuIcons[1]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_HITACHI2);
			MonIcons[0]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_COMPOSITE);
			MonIcons[1]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_RGB);
			hWndTabDialog= GetDlgItem(hDlg,IDC_CONFIGTAB); //get handle of Tabbed Dialog
			//get handles to all the sub panels in the control
			g_hWndConfig[0]=CreateDialog(EmuState.WindowInstance,MAKEINTRESOURCE(IDD_AUDIO),hWndTabDialog,(DLGPROC) AudioConfig);
			g_hWndConfig[1]=CreateDialog(EmuState.WindowInstance,MAKEINTRESOURCE(IDD_CPU),hWndTabDialog,(DLGPROC) CpuConfig);
			g_hWndConfig[2]=CreateDialog(EmuState.WindowInstance,MAKEINTRESOURCE(IDD_DISPLAY),hWndTabDialog,(DLGPROC) DisplayConfig);
			g_hWndConfig[3]=CreateDialog(EmuState.WindowInstance,MAKEINTRESOURCE(IDD_INPUT),hWndTabDialog,(DLGPROC) InputConfig);
			g_hWndConfig[4]=CreateDialog(EmuState.WindowInstance,MAKEINTRESOURCE(IDD_JOYSTICK),hWndTabDialog,(DLGPROC) JoyStickConfig);
			g_hWndConfig[5]=CreateDialog(EmuState.WindowInstance,MAKEINTRESOURCE(IDD_MISC),hWndTabDialog,(DLGPROC) MiscConfig);
			g_hWndConfig[6]=CreateDialog(EmuState.WindowInstance,MAKEINTRESOURCE(IDD_CASSETTE),hWndTabDialog,(DLGPROC) TapeConfig);
			g_hWndConfig[7]=CreateDialog(EmuState.WindowInstance,MAKEINTRESOURCE(IDD_BITBANGER),hWndTabDialog,(DLGPROC) BitBanger);
			//Set the title text for all tabs
			for (TabCount=0;TabCount<TABS;TabCount++)
			{
				Tabs.mask= TCIF_TEXT | TCIF_IMAGE;
				Tabs.iImage=-1;
				Tabs.pszText=TabTitles[TabCount];
				TabCtrl_InsertItem(hWndTabDialog,TabCount,&Tabs);
			}
			
			TabCtrl_SetCurSel(hWndTabDialog,0);	//Set Initial Tab to 0
			for (TabCount=0;TabCount<TABS;TabCount++)	//Hide All the Sub Panels
				ShowWindow(g_hWndConfig[TabCount],SW_HIDE);
			SetWindowPos(g_hWndConfig[0],HWND_TOP,10,30,0,0,SWP_NOSIZE|SWP_SHOWWINDOW);
			RefreshJoystickStatus();
		break;

		case WM_NOTIFY:
			if ((LOWORD(wParam))==IDC_CONFIGTAB)
			{
				SelectedTab=TabCtrl_GetCurSel(hWndTabDialog);
				for (TabCount=0;TabCount<TABS;TabCount++)
					ShowWindow(g_hWndConfig[TabCount],SW_HIDE);
				SetWindowPos(g_hWndConfig[SelectedTab],HWND_TOP,10,30,0,0,SWP_NOSIZE|SWP_SHOWWINDOW);		
			}
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
			case IDOK:
				hDlgBar=NULL;
				hDlgTape=NULL;
				EmuState.ResetPending=4;
				if ( (CurrentConfig.RamSize != TempConfig.RamSize) | (CurrentConfig.CpuType != TempConfig.CpuType) )
					EmuState.ResetPending=2;
				if ((CurrentConfig.SndOutDev != TempConfig.SndOutDev) | (CurrentConfig.AudioRate != TempConfig.AudioRate))
					SoundInit(EmuState.WindowHandle,SoundCards[TempConfig.SndOutDev].Guid,TempConfig.AudioRate);
				CurrentConfig=TempConfig;
				for (temp=0;temp<TABS;temp++)
					DestroyWindow(g_hWndConfig[temp]);

				SortKBtable(CurrentConfig.KeyMap);
				Right=TempRight;
				Left=TempLeft;
				SetStickNumbers(Left.DiDevice,Right.DiDevice);
			//	EndDialog(hDlg, LOWORD(wParam)); //TEST
				DestroyWindow(hDlg) ; //TEST
				EmuState.ConfigDialog=NULL;
				//if (EmuState.
				break;

			case IDAPPLY:
				EmuState.ResetPending=4;	
				if ( (CurrentConfig.RamSize != TempConfig.RamSize) | (CurrentConfig.CpuType != TempConfig.CpuType) )
					EmuState.ResetPending=2;
				if ((CurrentConfig.SndOutDev != TempConfig.SndOutDev) | (CurrentConfig.AudioRate != TempConfig.AudioRate))
					SoundInit(EmuState.WindowHandle,SoundCards[TempConfig.SndOutDev].Guid,TempConfig.AudioRate);
				CurrentConfig=TempConfig;
				SortKBtable(CurrentConfig.KeyMap);
				Right=TempRight;
				Left=TempLeft;
				SetStickNumbers(Left.DiDevice,Right.DiDevice);
			break;

			case IDCANCEL:
				for (temp=0;temp<TABS;temp++)
					DestroyWindow(g_hWndConfig[temp]);
				DestroyWindow(hDlg) ;
			//	EndDialog(hDlg, LOWORD(wParam)); //TEST
				EmuState.ConfigDialog=NULL;
				break;
			}

		break; //break WM_COMMAND
	} //End Switch
    return FALSE;
}

void GetIniFilePath( char *Path)
{
	strcpy(Path,IniFilePath);
	return;
}

void UpdateConfig (void)
{
	SetResize(CurrentConfig.Resize);
	SetAspect(CurrentConfig.Aspect);
	SetScanLines(CurrentConfig.ScanLines);
	SetFrameSkip(CurrentConfig.FrameSkip);
	SetAutoStart(CurrentConfig.AutoStart);
	SetSpeedThrottle(CurrentConfig.SpeedThrottle);
	SetCPUMultiplyer(CurrentConfig.CPUMultiplyer);
	SetRamSize(CurrentConfig.RamSize);
	SetCpuType(CurrentConfig.CpuType);
	SetMonitorType(CurrentConfig.MonitorType);
	SetCartAutoStart(CurrentConfig.CartAutoStart);
	if (CurrentConfig.RebootNow)
		DoReboot();
	CurrentConfig.RebootNow=0;
}

LRESULT CALLBACK CpuConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_CLOCKSPEED,TBM_SETRANGE,TRUE,MAKELONG(2,100) );
			sprintf(OutBuffer,"%2.3f Mhz",(float)TempConfig.CPUMultiplyer*.894);
			SendDlgItemMessage(hDlg,IDC_CLOCKDISPLAY,WM_SETTEXT,strlen(OutBuffer),(LPARAM)(LPCSTR)OutBuffer);
			SendDlgItemMessage(hDlg,IDC_CLOCKSPEED,TBM_SETPOS,TRUE,TempConfig.CPUMultiplyer);
			for (temp=0;temp<=3;temp++)
				SendDlgItemMessage(hDlg,Ramchoice[temp],BM_SETCHECK,(temp==TempConfig.RamSize),0);
			for (temp=0;temp<=1;temp++)
				SendDlgItemMessage(hDlg,Cpuchoice[temp],BM_SETCHECK,(temp==TempConfig.CpuType),0);
			SendDlgItemMessage(hDlg,IDC_CPUICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)CpuIcons[TempConfig.CpuType]);
		break;

		case WM_HSCROLL:
			TempConfig.CPUMultiplyer=(unsigned char) SendDlgItemMessage(hDlg,IDC_CLOCKSPEED,TBM_GETPOS,(WPARAM) 0, (WPARAM) 0);
			sprintf(OutBuffer,"%2.3f Mhz",(float)TempConfig.CPUMultiplyer*.894);
			SendDlgItemMessage(hDlg,IDC_CLOCKDISPLAY,WM_SETTEXT,strlen(OutBuffer),(LPARAM)(LPCSTR)OutBuffer);
			break;

		case WM_COMMAND:
			switch ( LOWORD(wParam))
			{
				case IDC_128K:
				case IDC_512K:
				case IDC_2M:
				case IDC_8M:
					for (temp=0;temp<=3;temp++)
						if (LOWORD(wParam)==Ramchoice[temp])
						{
							for (temp2=0;temp2<=3;temp2++)
								SendDlgItemMessage(hDlg,Ramchoice[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,Ramchoice[temp],BM_SETCHECK,1,0);
							TempConfig.RamSize=temp;
						}
				break;

				case IDC_6809:
				case IDC_6309:
					for (temp=0;temp<=1;temp++)
						if (LOWORD(wParam)==Cpuchoice[temp])
						{
							for (temp2=0;temp2<=1;temp2++)
							SendDlgItemMessage(hDlg,Cpuchoice[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,Cpuchoice[temp],BM_SETCHECK,1,0);
							TempConfig.CpuType=temp;
							SendDlgItemMessage(hDlg,IDC_CPUICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)CpuIcons[TempConfig.CpuType]);
						}
				break;
			}	//End switch LOWORD(wParam)
		break;	//Break WM_COMMAND
	}			//END switch (message)
	return(0);
}

LRESULT CALLBACK MiscConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_AUTOSTART,BM_SETCHECK,TempConfig.AutoStart,0);
			SendDlgItemMessage(hDlg,IDC_AUTOCART,BM_SETCHECK,TempConfig.CartAutoStart,0);
			break;

		case WM_COMMAND:
			TempConfig.AutoStart = (unsigned char)SendDlgItemMessage(hDlg,IDC_AUTOSTART,BM_GETCHECK,0,0);
			TempConfig.CartAutoStart = (unsigned char)SendDlgItemMessage(hDlg,IDC_AUTOCART,BM_GETCHECK,0,0);
			break;
	}
	return(0);
}

LRESULT CALLBACK TapeConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	HWND hCounter=NULL;
	CounterText.cbSize = sizeof(CHARFORMAT);
	CounterText.dwMask = CFM_BOLD | CFM_COLOR ;
	CounterText.dwEffects = CFE_BOLD;
	CounterText.crTextColor=RGB(255,255,255);


	ModeText.cbSize = sizeof(CHARFORMAT);
	ModeText.dwMask = CFM_BOLD | CFM_COLOR ;
	ModeText.dwEffects = CFE_BOLD;
	ModeText.crTextColor=RGB(255,0,0);



	switch (message)
	{
		case WM_INITDIALOG:
			TapeCounter=GetTapeCounter();
			sprintf(OutBuffer,"%i",TapeCounter);
			SendDlgItemMessage(hDlg,IDC_TCOUNT,WM_SETTEXT,strlen(OutBuffer),(LPARAM)(LPCSTR)OutBuffer);
			SendDlgItemMessage(hDlg,IDC_MODE,WM_SETTEXT,strlen(Tmodes[Tmode]),(LPARAM)(LPCSTR)Tmodes[Tmode]);
			GetTapeName(TapeFileName);
			SendDlgItemMessage(hDlg,IDC_TAPEFILE,WM_SETTEXT,strlen(TapeFileName),(LPARAM)(LPCSTR)TapeFileName);
//			hCounter=GetDlgItem(hDlg,IDC_TCOUNT);
//			SendMessage (hCounter, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0,0,0));
			SendDlgItemMessage(hDlg,IDC_TCOUNT,EM_SETBKGNDCOLOR ,0,(LPARAM)RGB(0,0,0)); 
			SendDlgItemMessage(hDlg,IDC_TCOUNT,EM_SETCHARFORMAT ,SCF_ALL,(LPARAM)&CounterText);
			SendDlgItemMessage(hDlg,IDC_MODE,EM_SETBKGNDCOLOR ,0,(LPARAM)RGB(0,0,0)); 
			SendDlgItemMessage(hDlg,IDC_MODE,EM_SETCHARFORMAT ,SCF_ALL,(LPARAM)&CounterText);
//			SendDlgItemMessage(hDlg,IDC_MODE,EM_SETCHARFORMAT ,SCF_ALL,(LPARAM)&ModeText);
			hDlgTape=hDlg;
		break;

		case WM_COMMAND:
			switch (LOWORD (wParam))
			{
				case IDC_PLAY:
					Tmode=PLAY;
					SetTapeMode(Tmode);
				break;
				case IDC_REC:
					Tmode=REC;
					SetTapeMode(Tmode);
				break;
				case IDC_STOP:
					Tmode=STOP;
					SetTapeMode(Tmode);
				break;

				case IDC_EJECT:
					Tmode=EJECT;
					SetTapeMode(Tmode);
				break;

				case IDC_RESET:
					TapeCounter=0;
					SetTapeCounter(TapeCounter);
				break;

				case IDC_TBROWSE:
					LoadTape();
					TapeCounter=0;
					SetTapeCounter(TapeCounter);
				break;

			}
		break;	//End WM_COMMAND
	}
	return(0);
}

LRESULT CALLBACK AudioConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned char Index=0;

	switch (message)
	{
		case WM_INITDIALOG:
			hDlgBar=hDlg;	//Save the handle to update sound bars
			SendDlgItemMessage(hDlg,IDC_PROGRESSLEFT,PBM_SETPOS,0,0);
			SendDlgItemMessage(hDlg,IDC_PROGRESSLEFT,PBM_SETRANGE32,0,0x7F);
			SendDlgItemMessage(hDlg,IDC_PROGRESSRIGHT,PBM_SETPOS,0,0);
			SendDlgItemMessage(hDlg,IDC_PROGRESSRIGHT,PBM_SETRANGE32,0,0x7F);
			SendDlgItemMessage(hDlg,IDC_PROGRESSLEFT,PBM_SETPOS,0,0);
			SendDlgItemMessage(hDlg,IDC_PROGRESSLEFT,PBM_SETBARCOLOR,0,0xFFFF); //bgr 
			SendDlgItemMessage(hDlg,IDC_PROGRESSRIGHT,PBM_SETBARCOLOR,0,0xFFFF); 
			SendDlgItemMessage(hDlg,IDC_PROGRESSLEFT,PBM_SETBKCOLOR,0,0);
			SendDlgItemMessage(hDlg,IDC_PROGRESSRIGHT,PBM_SETBKCOLOR,0,0);
			for (Index=0;Index<NumberOfSoundCards;Index++)
				SendDlgItemMessage(hDlg,IDC_SOUNDCARD,CB_ADDSTRING,(WPARAM)0,(LPARAM)SoundCards[Index].CardName);

			for (Index=0;Index<4;Index++)
				SendDlgItemMessage(hDlg,IDC_RATE,CB_ADDSTRING,(WPARAM)0,(LPARAM)RateList[Index]);

			SendDlgItemMessage(hDlg,IDC_RATE,CB_SETCURSEL,(WPARAM)TempConfig.AudioRate,(LPARAM)0);
			TempConfig.SndOutDev=0;
			for (Index=0;Index<NumberOfSoundCards;Index++)
				if (!strcmp(SoundCards[Index].CardName,TempConfig.SoundCardName))
					TempConfig.SndOutDev=Index;
			SendDlgItemMessage(hDlg,IDC_SOUNDCARD,CB_SETCURSEL,(WPARAM)TempConfig.SndOutDev,(LPARAM)0);
			//EnableWindow( GetDlgItem(hDlg,IDC_MUTE),SndCardPresent);
		break;

		case WM_COMMAND:
			TempConfig.SndOutDev=(unsigned char)SendDlgItemMessage(hDlg,IDC_SOUNDCARD,CB_GETCURSEL,0,0);
			TempConfig.AudioRate=(unsigned char)SendDlgItemMessage(hDlg,IDC_RATE,CB_GETCURSEL,0,0);
			strcpy(TempConfig.SoundCardName,SoundCards[TempConfig.SndOutDev].CardName);
		break;
	}
	return(0);
}

LRESULT CALLBACK DisplayConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:

			SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_SETRANGE,TRUE,MAKELONG(1,6) );
			SendDlgItemMessage(hDlg,IDC_SCANLINES,BM_SETCHECK,TempConfig.ScanLines,0);
			SendDlgItemMessage(hDlg,IDC_THROTTLE,BM_SETCHECK,TempConfig.SpeedThrottle,0);
			SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_SETPOS,TRUE,TempConfig.FrameSkip);
			SendDlgItemMessage(hDlg,IDC_RESIZE,BM_SETCHECK,TempConfig.Resize,0);
			SendDlgItemMessage(hDlg,IDC_ASPECT,BM_SETCHECK,TempConfig.Aspect,0);
			sprintf(OutBuffer,"%i",TempConfig.FrameSkip);
			SendDlgItemMessage(hDlg,IDC_FRAMEDISPLAY,WM_SETTEXT,strlen(OutBuffer),(LPARAM)(LPCSTR)OutBuffer);
			for (temp=0;temp<=1;temp++)
				SendDlgItemMessage(hDlg,Monchoice[temp],BM_SETCHECK,(temp==TempConfig.MonitorType),0);
			SendDlgItemMessage(hDlg,IDC_MONTYPE,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)MonIcons[TempConfig.MonitorType]);
		break;

		case WM_HSCROLL:
			TempConfig.FrameSkip=(unsigned char) SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_GETPOS,(WPARAM) 0, (WPARAM) 0);
			sprintf(OutBuffer,"%i",TempConfig.FrameSkip);
			SendDlgItemMessage(hDlg,IDC_FRAMEDISPLAY,WM_SETTEXT,strlen(OutBuffer),(LPARAM)(LPCSTR)OutBuffer);
		break;
		
		case WM_COMMAND:
			TempConfig.Resize = (unsigned char)SendDlgItemMessage(hDlg,IDC_RESIZE,BM_GETCHECK,0,0);
			TempConfig.Aspect = (unsigned char)SendDlgItemMessage(hDlg,IDC_ASPECT,BM_GETCHECK,0,0);
			TempConfig.ScanLines  = (unsigned char)SendDlgItemMessage(hDlg,IDC_SCANLINES,BM_GETCHECK,0,0);
			TempConfig.SpeedThrottle = (unsigned char)SendDlgItemMessage(hDlg,IDC_THROTTLE,BM_GETCHECK,0,0);
			switch (LOWORD (wParam))
			{
				case IDC_COMPOSITE:
				case IDC_RGB:
					for (temp=0;temp<=1;temp++)
						if (LOWORD(wParam)==Monchoice[temp])
						{
							for (temp2=0;temp2<=1;temp2++)
								SendDlgItemMessage(hDlg,Monchoice[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,Monchoice[temp],BM_SETCHECK,1,0);
							TempConfig.MonitorType=temp;
						}
					SendDlgItemMessage(hDlg,IDC_MONTYPE,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)MonIcons[TempConfig.MonitorType]);
				break;

			}	//End switch LOWORD(wParam)
		break;	//break WM_COMMAND
	}			//END switch Message
	return(0);
}
LRESULT CALLBACK InputConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	#define MAXPOS 100

	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_ADDSTRING,(WPARAM)0,(LPARAM)"Basic");
			SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_ADDSTRING,(WPARAM)0,(LPARAM)"OS-9");
		//	SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_ADDSTRING,(WPARAM)0,(LPARAM)"Untranslated");
		//	SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_ADDSTRING,(WPARAM)0,(LPARAM)"Custom");
			SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_SETCURSEL,(WPARAM)CurrentConfig.KeyMap,(LPARAM)0);
			break;

		case WM_COMMAND:
			TempConfig.KeyMap=(unsigned char)SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_GETCURSEL,0,0);
			break;

	}
	return(0);
}

LRESULT CALLBACK JoyStickConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int LeftJoyStick[6]={IDC_LEFT_LEFT,IDC_LEFT_RIGHT,IDC_LEFT_UP,IDC_LEFT_DOWN,IDC_LEFT_FIRE1,IDC_LEFT_FIRE2};
	static int RightJoyStick[6]={IDC_RIGHT_LEFT,IDC_RIGHT_RIGHT,IDC_RIGHT_UP,IDC_RIGHT_DOWN,IDC_RIGHT_FIRE1,IDC_RIGHT_FIRE2};
	static int LeftRadios[4]={IDC_LEFT_KEYBOARD,IDC_LEFT_USEMOUSE,IDC_LEFTAUDIO,IDC_LEFTJOYSTICK};
	static int RightRadios[4]={IDC_RIGHT_KEYBOARD,IDC_RIGHT_USEMOUSE,IDC_RIGHTAUDIO,IDC_RIGHTJOYSTICK};
	switch (message)
	{
		case WM_INITDIALOG:
			JoystickIcons[0]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_KEYBOARD);
			JoystickIcons[1]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_MOUSE);
			JoystickIcons[2]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_AUDIO);
			JoystickIcons[3]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_JOYSTICK);
			for (temp=0;temp<68;temp++)
				for (temp2=0;temp2<6;temp2++)
				{
					SendDlgItemMessage(hDlg,LeftJoyStick[temp2],CB_ADDSTRING,(WPARAM)0,(LPARAM)Entries[temp]);
					SendDlgItemMessage(hDlg,RightJoyStick[temp2],CB_ADDSTRING,(WPARAM)0,(LPARAM)Entries[temp]);
				}

			for (temp=0;temp<6;temp++)
				EnableWindow( GetDlgItem(hDlg,LeftJoyStick[temp]),(Left.UseMouse==0));
			for (temp=0;temp<6;temp++)
				EnableWindow( GetDlgItem(hDlg,RightJoyStick[temp]),(Right.UseMouse==0));

			EnableWindow( GetDlgItem(hDlg,IDC_LEFTAUDIODEVICE),(Left.UseMouse==2));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTAUDIODEVICE),(Right.UseMouse==2));
			EnableWindow( GetDlgItem(hDlg,IDC_LEFTJOYSTICKDEVICE),(Left.UseMouse==3));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTJOYSTICKDEVICE),(Right.UseMouse==3));

			EnableWindow( GetDlgItem(hDlg,IDC_LEFTJOYSTICK),(NumberofJoysticks>0));		//Grey the Joystick Radios if
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTJOYSTICK),(NumberofJoysticks>0));	//No Joysticks are present
			//populate joystick combo boxs
			for(temp=0;temp<NumberofJoysticks;temp++)
			{
				SendDlgItemMessage(hDlg,IDC_RIGHTJOYSTICKDEVICE,CB_ADDSTRING,(WPARAM)0,(LPARAM)StickName[temp]);
				SendDlgItemMessage(hDlg,IDC_LEFTJOYSTICKDEVICE,CB_ADDSTRING,(WPARAM)0,(LPARAM)StickName[temp]);
			}
			SendDlgItemMessage(hDlg,IDC_RIGHTJOYSTICKDEVICE,CB_SETCURSEL,(WPARAM)Right.DiDevice,(LPARAM)0);
			SendDlgItemMessage(hDlg,IDC_LEFTJOYSTICKDEVICE,CB_SETCURSEL,(WPARAM)Left.DiDevice,(LPARAM)0);

			SendDlgItemMessage(hDlg,LeftJoyStick[0],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Left.Left],(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[1],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Left.Right],(LPARAM)0);	
			SendDlgItemMessage(hDlg,LeftJoyStick[2],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Left.Up],(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[3],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Left.Down],(LPARAM)0);	
			SendDlgItemMessage(hDlg,LeftJoyStick[4],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Left.Fire1],(LPARAM)0);	
			SendDlgItemMessage(hDlg,LeftJoyStick[5],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Left.Fire2],(LPARAM)0);	
			SendDlgItemMessage(hDlg,RightJoyStick[0],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Right.Left],(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[1],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Right.Right],(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[2],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Right.Up],(LPARAM)0);	
			SendDlgItemMessage(hDlg,RightJoyStick[3],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Right.Down],(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[4],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Right.Fire1],(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[5],CB_SETCURSEL,(WPARAM)TranslateScan2Disp[Right.Fire2],(LPARAM)0);

			for (temp=0;temp<=3;temp++)
				SendDlgItemMessage(hDlg,LeftRadios[temp],BM_SETCHECK,temp==Left.UseMouse,0);

			for (temp=0;temp<=3;temp++)
				SendDlgItemMessage(hDlg,RightRadios[temp],BM_SETCHECK,temp==Right.UseMouse,0);
			TempLeft=Left;
			TempRight=Right;
			SendDlgItemMessage(hDlg,IDC_LEFTICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[Left.UseMouse]);
			SendDlgItemMessage(hDlg,IDC_RIGHTICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[Right.UseMouse]);
		break;

		case WM_COMMAND:
			for (temp=0;temp<=3;temp++)
				if (LOWORD(wParam)==LeftRadios[temp])
				{
					for (temp2=0;temp2<=3;temp2++)
						SendDlgItemMessage(hDlg,LeftRadios[temp2],BM_SETCHECK,0,0);
					SendDlgItemMessage(hDlg,LeftRadios[temp],BM_SETCHECK,1,0);
					TempLeft.UseMouse=temp;
				}

			for (temp=0;temp<=3;temp++)
				if (LOWORD(wParam)==RightRadios[temp])
				{
					for (temp2=0;temp2<=3;temp2++)
						SendDlgItemMessage(hDlg,RightRadios[temp2],BM_SETCHECK,0,0);
					SendDlgItemMessage(hDlg,RightRadios[temp],BM_SETCHECK,1,0);
					TempRight.UseMouse=temp;
				}

			for (temp=0;temp<6;temp++)
				EnableWindow( GetDlgItem(hDlg,LeftJoyStick[temp]),(TempLeft.UseMouse==0));

			for (temp=0;temp<6;temp++)
				EnableWindow( GetDlgItem(hDlg,RightJoyStick[temp]),(TempRight.UseMouse==0));

			EnableWindow( GetDlgItem(hDlg,IDC_LEFTAUDIODEVICE),(TempLeft.UseMouse==2));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTAUDIODEVICE),(TempRight.UseMouse==2));
			EnableWindow( GetDlgItem(hDlg,IDC_LEFTJOYSTICKDEVICE),(TempLeft.UseMouse==3));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTJOYSTICKDEVICE),(TempRight.UseMouse==3));

			TempLeft.Left=TranslateDisp2Scan[SendDlgItemMessage(hDlg,LeftJoyStick[0],CB_GETCURSEL,0,0)];
			TempLeft.Right=TranslateDisp2Scan[SendDlgItemMessage(hDlg,LeftJoyStick[1],CB_GETCURSEL,0,0)];
			TempLeft.Up=TranslateDisp2Scan[SendDlgItemMessage(hDlg,LeftJoyStick[2],CB_GETCURSEL,0,0)];
			TempLeft.Down=TranslateDisp2Scan[SendDlgItemMessage(hDlg,LeftJoyStick[3],CB_GETCURSEL,0,0)];
			TempLeft.Fire1=TranslateDisp2Scan[SendDlgItemMessage(hDlg,LeftJoyStick[4],CB_GETCURSEL,0,0)];
			TempLeft.Fire2=TranslateDisp2Scan[SendDlgItemMessage(hDlg,LeftJoyStick[5],CB_GETCURSEL,0,0)];

			TempRight.Left=TranslateDisp2Scan[SendDlgItemMessage(hDlg,RightJoyStick[0],CB_GETCURSEL,0,0)];
			TempRight.Right=TranslateDisp2Scan[SendDlgItemMessage(hDlg,RightJoyStick[1],CB_GETCURSEL,0,0)];
			TempRight.Up=TranslateDisp2Scan[SendDlgItemMessage(hDlg,RightJoyStick[2],CB_GETCURSEL,0,0)];
			TempRight.Down=TranslateDisp2Scan[SendDlgItemMessage(hDlg,RightJoyStick[3],CB_GETCURSEL,0,0)];
			TempRight.Fire1=TranslateDisp2Scan[SendDlgItemMessage(hDlg,RightJoyStick[4],CB_GETCURSEL,0,0)];
			TempRight.Fire2=TranslateDisp2Scan[SendDlgItemMessage(hDlg,RightJoyStick[5],CB_GETCURSEL,0,0)];
			
			TempRight.DiDevice=(unsigned char)SendDlgItemMessage(hDlg,IDC_RIGHTJOYSTICKDEVICE,CB_GETCURSEL,0,0);
			TempLeft.DiDevice=(unsigned char)SendDlgItemMessage(hDlg,IDC_LEFTJOYSTICKDEVICE,CB_GETCURSEL,0,0);	//Fix Me;

			SendDlgItemMessage(hDlg,IDC_LEFTICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[TempLeft.UseMouse]);
			SendDlgItemMessage(hDlg,IDC_RIGHTICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[TempRight.UseMouse]);
		break;
	} //END switch message
	return(0);
}

void BuildKeyboardTable()
{
	short Index=0,Index2=0;
	char FullMapPath[MAX_PATH]="";
	HANDLE hr=0;

	for (Index=0;Index<84;Index++)
		for (Index2=84;Index2>=0;Index2--)
			if (Index2 == TranslateScan2Disp[Index])
				TranslateDisp2Scan[Index2]=(unsigned char)Index;
	TranslateDisp2Scan[0]=0;
	TranslateDisp2Scan[51]=42;	//Left and Right Shift 

//	strcpy(FullMapPath,ExecDirectory);	
//	strcat(FullMapPath,CurrentConfig.KeymapFile);
//	hr=CreateFile(FullMapPath,NULL,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
//	if (hr==INVALID_HANDLE_VALUE)	//KeyMap.ini doesn't exist
//	{

	for (Index=0;Index<=3;Index++)
		for (Index2=0;Index2<100;Index2++)
		{
			RawTable[Index][Index2].ScanCode1=0;
			RawTable[Index][Index2].ScanCode2=0;
			RawTable[Index][Index2].Col1=0;
			RawTable[Index][Index2].Col2=0;
			RawTable[Index][Index2].Row1=0;
			RawTable[Index][Index2].Row2=0;
		}


//DECB
	RawTable[0][0].ScanCode1=30; //A
	RawTable[0][0].Col1=1;
	RawTable[0][0].Row1=1;
	RawTable[0][1].ScanCode1=48; //B
	RawTable[0][1].Col1=2;
	RawTable[0][1].Row1=1;	
	RawTable[0][2].ScanCode1=46; //C
	RawTable[0][2].Col1=3;
	RawTable[0][2].Row1=1;
	RawTable[0][3].ScanCode1=32; //D
	RawTable[0][3].Col1=4;
	RawTable[0][3].Row1=1;
	RawTable[0][4].ScanCode1=18; //E
	RawTable[0][4].Col1=5;
	RawTable[0][4].Row1=1;
	RawTable[0][5].ScanCode1=33; //F
	RawTable[0][5].Col1=6;
	RawTable[0][5].Row1=1;
	RawTable[0][6].ScanCode1=34; //G
	RawTable[0][6].Col1=7;
	RawTable[0][6].Row1=1;
	RawTable[0][7].ScanCode1=35; //H
	RawTable[0][7].Col1=0;
	RawTable[0][7].Row1=2;
	RawTable[0][8].ScanCode1=23; //I
	RawTable[0][8].Col1=1;
	RawTable[0][8].Row1=2;
	RawTable[0][9].ScanCode1=36; //J
	RawTable[0][9].Col1=2;
	RawTable[0][9].Row1=2;
	RawTable[0][10].ScanCode1=37; //K
	RawTable[0][10].Col1=3;
	RawTable[0][10].Row1=2;
	RawTable[0][11].ScanCode1=38; //L
	RawTable[0][11].Col1=4;
	RawTable[0][11].Row1=2;
	RawTable[0][12].ScanCode1=50; //M
	RawTable[0][12].Col1=5;
	RawTable[0][12].Row1=2;
	RawTable[0][13].ScanCode1=49; //N
	RawTable[0][13].Col1=6;
	RawTable[0][13].Row1=2;
	RawTable[0][14].ScanCode1=24; //O
	RawTable[0][14].Col1=7;
	RawTable[0][14].Row1=2;
	RawTable[0][15].ScanCode1=25; //P
	RawTable[0][15].Col1=0;
	RawTable[0][15].Row1=4;
	RawTable[0][16].ScanCode1=16; //Q
	RawTable[0][16].Col1=1;
	RawTable[0][16].Row1=4;
	RawTable[0][17].ScanCode1=19; //R
	RawTable[0][17].Col1=2;
	RawTable[0][17].Row1=4;
	RawTable[0][18].ScanCode1=31; //S
	RawTable[0][18].Col1=3;
	RawTable[0][18].Row1=4;
	RawTable[0][19].ScanCode1=20; //T
	RawTable[0][19].Col1=4;
	RawTable[0][19].Row1=4;
	RawTable[0][20].ScanCode1=22; //U
	RawTable[0][20].Col1=5;
	RawTable[0][20].Row1=4;
	RawTable[0][21].ScanCode1=47; //V
	RawTable[0][21].Col1=6;
	RawTable[0][21].Row1=4;
	RawTable[0][22].ScanCode1=17; //W
	RawTable[0][22].Col1=7;
	RawTable[0][22].Row1=4;
	RawTable[0][23].ScanCode1=45; //X
	RawTable[0][23].Col1=0;
	RawTable[0][23].Row1=8;
	RawTable[0][24].ScanCode1=21; //Y
	RawTable[0][24].Col1=1;
	RawTable[0][24].Row1=8;
	RawTable[0][25].ScanCode1=44; //Z
	RawTable[0][25].Col1=2;
	RawTable[0][25].Row1=8;
	RawTable[0][26].ScanCode1=72; //Up Arrow
	RawTable[0][26].Col1=3;
	RawTable[0][26].Row1=8;
	RawTable[0][27].ScanCode1=80; //Down Arrow
	RawTable[0][27].Col1=4;
	RawTable[0][27].Row1=8;
	RawTable[0][28].ScanCode1=75; //Left Arrow
	RawTable[0][28].Col1=5;
	RawTable[0][28].Row1=8;
	RawTable[0][29].ScanCode1=77; //Right Arrow
	RawTable[0][29].Col1=6;
	RawTable[0][29].Row1=8;
	RawTable[0][30].ScanCode1=57; //Space
	RawTable[0][30].Col1=7;
	RawTable[0][30].Row1=8;
	RawTable[0][31].ScanCode1=11; //0
	RawTable[0][31].Col1=0;
	RawTable[0][31].Row1=16;
	RawTable[0][32].ScanCode1=2; //1
	RawTable[0][32].Col1=1;
	RawTable[0][32].Row1=16;
	RawTable[0][33].ScanCode1=3; //2
	RawTable[0][33].Col1=2;
	RawTable[0][33].Row1=16;
	RawTable[0][34].ScanCode1=4; //3
	RawTable[0][34].Col1=3;
	RawTable[0][34].Row1=16;
	RawTable[0][35].ScanCode1=5; //4
	RawTable[0][35].Col1=4;
	RawTable[0][35].Row1=16;
	RawTable[0][36].ScanCode1=6; //5
	RawTable[0][36].Col1=5;
	RawTable[0][36].Row1=16;
	RawTable[0][37].ScanCode1=7; //6
	RawTable[0][37].Col1=6;
	RawTable[0][37].Row1=16;
	RawTable[0][38].ScanCode1=8; //7
	RawTable[0][38].Col1=7;
	RawTable[0][38].Row1=16;
	RawTable[0][39].ScanCode1=9; //8
	RawTable[0][39].Col1=0;
	RawTable[0][39].Row1=32;
	RawTable[0][40].ScanCode1=10; //9
	RawTable[0][40].Col1=1;
	RawTable[0][40].Row1=32;
	RawTable[0][41].ScanCode1=39; //:
	RawTable[0][41].ScanCode2=42;
	RawTable[0][41].Col1=2;
	RawTable[0][41].Row1=32;
	RawTable[0][42].ScanCode1=39; //;
	RawTable[0][42].Col1=3;
	RawTable[0][42].Row1=32;
	RawTable[0][43].ScanCode1=51; //,
	RawTable[0][43].Col1=4;
	RawTable[0][43].Row1=32;
	RawTable[0][44].ScanCode1=12; //-
	RawTable[0][44].Col1=5;
	RawTable[0][44].Row1=32;
	RawTable[0][45].ScanCode1=52; //.
	RawTable[0][45].Col1=6;
	RawTable[0][45].Row1=32;
	RawTable[0][46].ScanCode1=53; // "/"
	RawTable[0][46].Col1=7;
	RawTable[0][46].Row1=32;
	RawTable[0][47].ScanCode1=28; //ENTER
	RawTable[0][47].Col1=0;
	RawTable[0][47].Row1=64;
	RawTable[0][48].ScanCode1=71; //HOME (CLEAR)
	RawTable[0][48].Col1=1;
	RawTable[0][48].Row1=64;
	RawTable[0][49].ScanCode1=79; //ESCAPE (BREAK) was 1 ESC
	RawTable[0][49].Col1=2;
	RawTable[0][49].Row1=64;
	RawTable[0][50].ScanCode1=56; //ALT
	RawTable[0][50].Col1=3;
	RawTable[0][50].Row1=64;
	RawTable[0][51].ScanCode1=29; //CONTROL
	RawTable[0][51].Col1=4;
	RawTable[0][51].Row1=64;
	RawTable[0][52].ScanCode1=59; //F1
	RawTable[0][52].Col1=5;
	RawTable[0][52].Row1=64;
	RawTable[0][53].ScanCode1=60; //F2
	RawTable[0][53].Col1=6;
	RawTable[0][53].Row1=64;
	RawTable[0][54].ScanCode1=2; //!
	RawTable[0][54].ScanCode2=42;
	RawTable[0][54].Col1=1;
	RawTable[0][54].Row1=16;
	RawTable[0][54].Col2=7;
	RawTable[0][54].Row2=64;
	RawTable[0][55].ScanCode1=40; //@
	RawTable[0][55].ScanCode2=42;
	RawTable[0][55].Col1=2;
	RawTable[0][55].Row1=16;
	RawTable[0][55].Col2=7;
	RawTable[0][55].Row2=64;
	RawTable[0][56].ScanCode1=8; //&
	RawTable[0][56].ScanCode2=42;
	RawTable[0][56].Col1=6;
	RawTable[0][56].Row1=16;
	RawTable[0][56].Col2=7;
	RawTable[0][56].Row2=64;
	RawTable[0][57].ScanCode1=7; //^ Caret
	RawTable[0][57].ScanCode2=42;
	RawTable[0][57].Col1=3;
	RawTable[0][57].Row1=8;
	RawTable[0][58].ScanCode1=40; //'
	RawTable[0][58].Col1=7;
	RawTable[0][58].Row1=16;
	RawTable[0][58].Col2=7;
	RawTable[0][58].Row2=64;
	RawTable[0][59].ScanCode1=10; // (
	RawTable[0][59].ScanCode2=42;
	RawTable[0][59].Col1=0;
	RawTable[0][59].Row1=32;
	RawTable[0][59].Col2=7;
	RawTable[0][59].Row2=64;
	RawTable[0][60].ScanCode1=11; //)
	RawTable[0][60].ScanCode2=42;
	RawTable[0][60].Col1=1;
	RawTable[0][60].Row1=32;
	RawTable[0][60].Col2=7;
	RawTable[0][60].Row2=64;
	RawTable[0][61].ScanCode1=9; //*
	RawTable[0][61].ScanCode2=42;
	RawTable[0][61].Col1=2;
	RawTable[0][61].Row1=32;
	RawTable[0][61].Col2=7;
	RawTable[0][61].Row2=64;
	RawTable[0][62].ScanCode1=13; //+
	RawTable[0][62].ScanCode2=42;
	RawTable[0][62].Col1=3;
	RawTable[0][62].Row1=32;
	RawTable[0][62].Col2=7;
	RawTable[0][62].Row2=64;
	RawTable[0][63].ScanCode1=51; //<
	RawTable[0][63].ScanCode2=42;
	RawTable[0][63].Col1=4;
	RawTable[0][63].Row1=32;
	RawTable[0][63].Col2=7;
	RawTable[0][63].Row2=64;
	RawTable[0][64].ScanCode1=13; //=
	RawTable[0][64].Col1=5;
	RawTable[0][64].Row1=32;
	RawTable[0][64].Col2=7;
	RawTable[0][64].Row2=64;
	RawTable[0][65].ScanCode1=58; //Caps Lock
	RawTable[0][65].Col1=0;
	RawTable[0][65].Row1=16;
	RawTable[0][65].Col2=7;
	RawTable[0][65].Row2=64;
	RawTable[0][66].ScanCode1=3;
	RawTable[0][66].ScanCode2=42;
	RawTable[0][66].Col1=0;
	RawTable[0][66].Row1=1;
	RawTable[0][67].ScanCode1=14; //Back Space
	RawTable[0][67].Col1=5;
	RawTable[0][67].Row1=8;
	RawTable[0][68].ScanCode1=26; // [
	RawTable[0][68].Col1=7;
	RawTable[0][68].Row1=64;
	RawTable[0][68].Col2=4;
	RawTable[0][68].Row2=8;
	RawTable[0][69].ScanCode1=27; // ]
	RawTable[0][69].Col1=7;
	RawTable[0][69].Row1=64;
	RawTable[0][69].Col2=6;
	RawTable[0][69].Row2=8;
	RawTable[0][99].ScanCode1=42; //SHIFT
	RawTable[0][99].Col1=7;
	RawTable[0][99].Row1=64;
//OS9
	RawTable[1][0].ScanCode1=30; //A
	RawTable[1][0].Col1=1;
	RawTable[1][0].Row1=1;
	RawTable[1][1].ScanCode1=48; //B
	RawTable[1][1].Col1=2;
	RawTable[1][1].Row1=1;	
	RawTable[1][2].ScanCode1=46; //C
	RawTable[1][2].Col1=3;
	RawTable[1][2].Row1=1;
	RawTable[1][3].ScanCode1=32; //D
	RawTable[1][3].Col1=4;
	RawTable[1][3].Row1=1;
	RawTable[1][4].ScanCode1=18; //E
	RawTable[1][4].Col1=5;
	RawTable[1][4].Row1=1;
	RawTable[1][5].ScanCode1=33; //F
	RawTable[1][5].Col1=6;
	RawTable[1][5].Row1=1;
	RawTable[1][6].ScanCode1=34; //G
	RawTable[1][6].Col1=7;
	RawTable[1][6].Row1=1;
	RawTable[1][7].ScanCode1=35; //H
	RawTable[1][7].Col1=0;
	RawTable[1][7].Row1=2;
	RawTable[1][8].ScanCode1=23; //I
	RawTable[1][8].Col1=1;
	RawTable[1][8].Row1=2;
	RawTable[1][9].ScanCode1=36; //J
	RawTable[1][9].Col1=2;
	RawTable[1][9].Row1=2;
	RawTable[1][10].ScanCode1=37; //K
	RawTable[1][10].Col1=3;
	RawTable[1][10].Row1=2;
	RawTable[1][11].ScanCode1=38; //L
	RawTable[1][11].Col1=4;
	RawTable[1][11].Row1=2;
	RawTable[1][12].ScanCode1=50; //M
	RawTable[1][12].Col1=5;
	RawTable[1][12].Row1=2;
	RawTable[1][13].ScanCode1=49; //N
	RawTable[1][13].Col1=6;
	RawTable[1][13].Row1=2;
	RawTable[1][14].ScanCode1=24; //O
	RawTable[1][14].Col1=7;
	RawTable[1][14].Row1=2;
	RawTable[1][15].ScanCode1=25; //P
	RawTable[1][15].Col1=0;
	RawTable[1][15].Row1=4;
	RawTable[1][16].ScanCode1=16; //Q
	RawTable[1][16].Col1=1;
	RawTable[1][16].Row1=4;
	RawTable[1][17].ScanCode1=19; //R
	RawTable[1][17].Col1=2;
	RawTable[1][17].Row1=4;
	RawTable[1][18].ScanCode1=31; //S
	RawTable[1][18].Col1=3;
	RawTable[1][18].Row1=4;
	RawTable[1][19].ScanCode1=20; //T
	RawTable[1][19].Col1=4;
	RawTable[1][19].Row1=4;
	RawTable[1][20].ScanCode1=22; //U
	RawTable[1][20].Col1=5;
	RawTable[1][20].Row1=4;
	RawTable[1][21].ScanCode1=47; //V
	RawTable[1][21].Col1=6;
	RawTable[1][21].Row1=4;
	RawTable[1][22].ScanCode1=17; //W
	RawTable[1][22].Col1=7;
	RawTable[1][22].Row1=4;
	RawTable[1][23].ScanCode1=45; //X
	RawTable[1][23].Col1=0;
	RawTable[1][23].Row1=8;
	RawTable[1][24].ScanCode1=21; //Y
	RawTable[1][24].Col1=1;
	RawTable[1][24].Row1=8;
	RawTable[1][25].ScanCode1=44; //Z
	RawTable[1][25].Col1=2;
	RawTable[1][25].Row1=8;
	RawTable[1][26].ScanCode1=72; //Up Arrow
	RawTable[1][26].Col1=3;
	RawTable[1][26].Row1=8;
	RawTable[1][27].ScanCode1=80; //Down Arrow
	RawTable[1][27].Col1=4;
	RawTable[1][27].Row1=8;
	RawTable[1][28].ScanCode1=75; //Left Arrow
	RawTable[1][28].Col1=5;
	RawTable[1][28].Row1=8;
	RawTable[1][29].ScanCode1=77; //Right Arrow
	RawTable[1][29].Col1=6;
	RawTable[1][29].Row1=8;
	RawTable[1][30].ScanCode1=57; //Space
	RawTable[1][30].Col1=7;
	RawTable[1][30].Row1=8;
	RawTable[1][31].ScanCode1=11; //0
	RawTable[1][31].Col1=0;
	RawTable[1][31].Row1=16;
	RawTable[1][32].ScanCode1=2; //1
	RawTable[1][32].Col1=1;
	RawTable[1][32].Row1=16;
	RawTable[1][33].ScanCode1=3; //2
	RawTable[1][33].Col1=2;
	RawTable[1][33].Row1=16;
	RawTable[1][34].ScanCode1=4; //3
	RawTable[1][34].Col1=3;
	RawTable[1][34].Row1=16;
	RawTable[1][35].ScanCode1=5; //4
	RawTable[1][35].Col1=4;
	RawTable[1][35].Row1=16;
	RawTable[1][36].ScanCode1=6; //5
	RawTable[1][36].Col1=5;
	RawTable[1][36].Row1=16;
	RawTable[1][37].ScanCode1=7; //6
	RawTable[1][37].Col1=6;
	RawTable[1][37].Row1=16;
	RawTable[1][38].ScanCode1=8; //7
	RawTable[1][38].Col1=7;
	RawTable[1][38].Row1=16;
	RawTable[1][39].ScanCode1=9; //8
	RawTable[1][39].Col1=0;
	RawTable[1][39].Row1=32;
	RawTable[1][40].ScanCode1=10; //9
	RawTable[1][40].Col1=1;
	RawTable[1][40].Row1=32;
	RawTable[1][41].ScanCode1=39; //:
	RawTable[1][41].ScanCode2=42;
	RawTable[1][41].Col1=2;
	RawTable[1][41].Row1=32;
	RawTable[1][42].ScanCode1=39; //;
	RawTable[1][42].Col1=3;
	RawTable[1][42].Row1=32;
	RawTable[1][43].ScanCode1=51; //,
	RawTable[1][43].Col1=4;
	RawTable[1][43].Row1=32;
	RawTable[1][44].ScanCode1=12; //-
	RawTable[1][44].Col1=5;
	RawTable[1][44].Row1=32;
	RawTable[1][45].ScanCode1=52; //.
	RawTable[1][45].Col1=6;
	RawTable[1][45].Row1=32;
	RawTable[1][46].ScanCode1=53; // "/"
	RawTable[1][46].Col1=7;
	RawTable[1][46].Row1=32;
	RawTable[1][47].ScanCode1=28; //ENTER
	RawTable[1][47].Col1=0;
	RawTable[1][47].Row1=64;
	RawTable[1][48].ScanCode1=71; //HOME (CLEAR)
	RawTable[1][48].Col1=1;
	RawTable[1][48].Row1=64;
	RawTable[1][49].ScanCode1=79; //ESCAPE (BREAK) was 1 ESC
	RawTable[1][49].Col1=2;
	RawTable[1][49].Row1=64;
	RawTable[1][50].ScanCode1=56; //ALT
	RawTable[1][50].Col1=3;
	RawTable[1][50].Row1=64;
	RawTable[1][51].ScanCode1=29; //CONTROL
	RawTable[1][51].Col1=4;
	RawTable[1][51].Row1=64;
	RawTable[1][52].ScanCode1=59; //F1
	RawTable[1][52].Col1=5;
	RawTable[1][52].Row1=64;
	RawTable[1][53].ScanCode1=60; //F2
	RawTable[1][53].Col1=6;
	RawTable[1][53].Row1=64;
	RawTable[1][54].ScanCode1=2; //!
	RawTable[1][54].ScanCode2=42;
	RawTable[1][54].Col1=1;
	RawTable[1][54].Row1=16;
	RawTable[1][54].Col2=7;
	RawTable[1][54].Row2=64;
	RawTable[1][55].ScanCode1=40; //@
	RawTable[1][55].ScanCode2=42;
	RawTable[1][55].Col1=2;
	RawTable[1][55].Row1=16;
	RawTable[1][55].Col2=7;
	RawTable[1][55].Row2=64;
	RawTable[1][56].ScanCode1=8; //&
	RawTable[1][56].ScanCode2=42;
	RawTable[1][56].Col1=6;
	RawTable[1][56].Row1=16;
	RawTable[1][56].Col2=7;
	RawTable[1][56].Row2=64;
	RawTable[1][57].ScanCode1=7; //^ Caret
	RawTable[1][57].ScanCode2=42;
	RawTable[1][57].Col1=7;
	RawTable[1][57].Row1=16;
	RawTable[1][57].Col2=4;
	RawTable[1][57].Row2=64;
	RawTable[1][58].ScanCode1=40; //'
	RawTable[1][58].Col1=7;
	RawTable[1][58].Row1=16;
	RawTable[1][58].Col2=7;
	RawTable[1][58].Row2=64;
	RawTable[1][59].ScanCode1=10; // (
	RawTable[1][59].ScanCode2=42;
	RawTable[1][59].Col1=0;
	RawTable[1][59].Row1=32;
	RawTable[1][59].Col2=7;
	RawTable[1][59].Row2=64;
	RawTable[1][60].ScanCode1=11; //)
	RawTable[1][60].ScanCode2=42;
	RawTable[1][60].Col1=1;
	RawTable[1][60].Row1=32;
	RawTable[1][60].Col2=7;
	RawTable[1][60].Row2=64;
	RawTable[1][61].ScanCode1=9; //*
	RawTable[1][61].ScanCode2=42;
	RawTable[1][61].Col1=2;
	RawTable[1][61].Row1=32;
	RawTable[1][61].Col2=7;
	RawTable[1][61].Row2=64;
	RawTable[1][62].ScanCode1=13; //+
	RawTable[1][62].ScanCode2=42;
	RawTable[1][62].Col1=3;
	RawTable[1][62].Row1=32;
	RawTable[1][62].Col2=7;
	RawTable[1][62].Row2=64;
	RawTable[1][63].ScanCode1=51; //<
	RawTable[1][63].ScanCode2=42;
	RawTable[1][63].Col1=4;
	RawTable[1][63].Row1=32;
	RawTable[1][63].Col2=7;
	RawTable[1][63].Row2=64;
	RawTable[1][64].ScanCode1=13; //=
	RawTable[1][64].Col1=5;
	RawTable[1][64].Row1=32;
	RawTable[1][64].Col2=7;
	RawTable[1][64].Row2=64;
	RawTable[1][65].ScanCode1=58; //Caps Lock
	RawTable[1][65].Col1=0;
	RawTable[1][65].Row1=16;
	RawTable[1][65].Col2=4;
	RawTable[1][65].Row2=64;
	RawTable[1][66].ScanCode1=3;
	RawTable[1][66].ScanCode2=42;
	RawTable[1][66].Col1=0;
	RawTable[1][66].Row1=1;
	RawTable[1][67].ScanCode1=14; //Back Space
	RawTable[1][67].Col1=5;
	RawTable[1][67].Row1=8;
	RawTable[1][68].ScanCode1=26; // [
	RawTable[1][68].Col1=4;
	RawTable[1][68].Row1=64;
	RawTable[1][68].Col2=0;
	RawTable[1][68].Row2=32;
	RawTable[1][69].ScanCode1=27; // ]
	RawTable[1][69].Col1=4;
	RawTable[1][69].Row1=64;
	RawTable[1][69].Col2=1;
	RawTable[1][69].Row2=32;
	RawTable[1][70].ScanCode1=42; // {
	RawTable[1][70].ScanCode2=26;
	RawTable[1][70].Col1=4;
	RawTable[1][70].Row1=64;
	RawTable[1][70].Col2=4;
	RawTable[1][70].Row2=32;
	RawTable[1][71].ScanCode1=42; // }
	RawTable[1][71].ScanCode2=27;
	RawTable[1][71].Col1=4;
	RawTable[1][71].Row1=64;
	RawTable[1][71].Col2=6;
	RawTable[1][71].Row2=32;
	RawTable[1][72].ScanCode1=43;	//  BackSlash
	RawTable[1][72].Col1=7;
	RawTable[1][72].Row1=32;
	RawTable[1][72].Col2=4;
	RawTable[1][72].Row2=64;
	RawTable[1][73].ScanCode1=43;	//  | Pipe
	RawTable[1][73].ScanCode2=42;
	RawTable[1][73].Col1=1;
	RawTable[1][73].Row1=16;
	RawTable[1][73].Col2=4;
	RawTable[1][73].Row2=64;
	RawTable[1][74].ScanCode1=12;	//  _ UnderScore Ctrl -
	RawTable[1][74].ScanCode2=42;
	RawTable[1][74].Col1=5;
	RawTable[1][74].Row1=32;
	RawTable[1][74].Col2=4;
	RawTable[1][74].Row2=64;
	RawTable[1][75].ScanCode1=41;	//  ~
	RawTable[1][75].ScanCode2=42;
	RawTable[1][75].Col1=3;
	RawTable[1][75].Row1=16;
	RawTable[1][75].Col2=4;
	RawTable[1][75].Row2=64;
	RawTable[1][99].ScanCode1=42; //SHIFT
	RawTable[1][99].Col1=7;
	RawTable[1][99].Row1=64;
	return;
}

void SortKBtable(unsigned char KeyBoardMode) //Any SHIFT+ [char] entries need to be placed first
{
	unsigned short Index1=0,Index2=0;

	for (Index1=0;Index1<=100;Index1++)		//Shift must be first
	{
		if (RawTable[KeyBoardMode][Index1].ScanCode2==42)
		{
			RawTable[KeyBoardMode][Index1].ScanCode2=RawTable[KeyBoardMode][Index1].ScanCode1;
			RawTable[KeyBoardMode][Index1].ScanCode1=42;
		}

		if ((RawTable[KeyBoardMode][Index1].ScanCode1==0) & (RawTable[KeyBoardMode][Index1].ScanCode2!=0) )
		{
			RawTable[KeyBoardMode][Index1].ScanCode1=RawTable[KeyBoardMode][Index1].ScanCode2;
			RawTable[KeyBoardMode][Index1].ScanCode2=0;
		}
	}

	for (Index1=0;Index1<=100;Index1++)
		if ( RawTable[KeyBoardMode][Index1].ScanCode1==0)
		{
			RawTable[KeyBoardMode][Index1].Col1=0;
			RawTable[KeyBoardMode][Index1].Col2=0;
			RawTable[KeyBoardMode][Index1].Row1=0;
			RawTable[KeyBoardMode][Index1].Row2=0;
		}
	for (Index2=0;Index2<=100;Index2++)
	{
		Table[Index2].ScanCode1=0;
		Table[Index2].ScanCode2=0;
		Table[Index2].Col1=0;
		Table[Index2].Col2=0;
		Table[Index2].Row1=0;
		Table[Index2].Row2=0;
	}
	Index2=0;
	for (Index1=0;Index1<=100;Index1++)		//Double key combos first
		if ( (RawTable[KeyBoardMode][Index1].ScanCode1 !=0) & (RawTable[KeyBoardMode][Index1].ScanCode2 !=0))
			Table[Index2++]=RawTable[KeyBoardMode][Index1];

	for (Index1=0;Index1<=100;Index1++)
		if ( (RawTable[KeyBoardMode][Index1].ScanCode1 !=0) & (RawTable[KeyBoardMode][Index1].ScanCode2 ==0) & (RawTable[KeyBoardMode][Index1].ScanCode1 !=42) )
			Table[Index2++]=RawTable[KeyBoardMode][Index1];
	Table[Index2].ScanCode1=42;	//Shift must be last
	Table[Index2].Col1=7;
	Table[Index2].Row1=64;
	return;
}

unsigned char XY2Disp (unsigned char Row,unsigned char Col)
{
	switch (Row)
	{
	case 0:
		return(0);
	case 1:
		return(1+Col);
	case 2:
		return(9+Col);
	case 4:
		return(17+Col);
	case 8:
		return (25+Col);
	case 16:
		return(33+Col);
	case 32:
		return (41+Col);
	case 64:
		return (49+Col);
	default:
		return (0);
	}
}

void Disp2XY(unsigned char *Col,unsigned char *Row,unsigned char Disp)
{
	unsigned char temp=0;
	if (Disp ==0)
	{
		Col=0;
		Row=0;
		return;
	}
	Disp-=1;
	temp= Disp & 56;
	temp = temp >>3;
	*Row = 1<<temp;
	*Col=Disp & 7;
return;
}

void ReadKeymapFile(char *KeyFile)
{
	unsigned char MapBuffer[MAXPOS*6]="";
	unsigned int Index=0,KeyIndex=0;
	HANDLE MapFile=NULL;
	unsigned long BytesMoved=0;
	MapFile = CreateFile(KeyFile,GENERIC_READ ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	ReadFile(MapFile,MapBuffer,MAXPOS*6,&BytesMoved,NULL);
	CloseHandle(MapFile);
	for (KeyIndex=0;KeyIndex<MAXPOS;KeyIndex++)
	{
		RawTable[3][KeyIndex].ScanCode1=MapBuffer[Index++];
		RawTable[3][KeyIndex].ScanCode2=MapBuffer[Index++];
		RawTable[3][KeyIndex].Col1=MapBuffer[Index++];
		RawTable[3][KeyIndex].Col2=MapBuffer[Index++];
		RawTable[3][KeyIndex].Row1=MapBuffer[Index++];
		RawTable[3][KeyIndex].Row2=MapBuffer[Index++];
	}
	return;
}

void WriteKeymapFile(char *KeyFile)
{
	unsigned char MapBuffer[MAXPOS*6]="";
	unsigned int Index=0,KeyIndex=0;
	HANDLE MapFile=NULL;
	unsigned long BytesMoved=0;
	for (KeyIndex=0;KeyIndex<MAXPOS;KeyIndex++)
	{
		MapBuffer[Index++]=RawTable[3][KeyIndex].ScanCode1;
		MapBuffer[Index++]=RawTable[3][KeyIndex].ScanCode2;
		MapBuffer[Index++]=RawTable[3][KeyIndex].Col1;
		MapBuffer[Index++]=RawTable[3][KeyIndex].Col2;
		MapBuffer[Index++]=RawTable[3][KeyIndex].Row1;
		MapBuffer[Index++]=RawTable[3][KeyIndex].Row2;
	}
	MapFile = CreateFile(KeyFile,GENERIC_READ | GENERIC_WRITE,0,0,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	if (MapFile == INVALID_HANDLE_VALUE)	
		return;
	WriteFile(MapFile,MapBuffer,MAXPOS*6,&BytesMoved,NULL);
	CloseHandle(MapFile);
	return;
}

void RefreshJoystickStatus(void)
{
	unsigned char Index=0;
	bool Temp=false;

	NumberofJoysticks = EnumerateJoysticks();
	for (Index=0;Index<NumberofJoysticks;Index++)
		Temp=InitJoyStick (Index);

	if (Right.DiDevice>(NumberofJoysticks-1))
		Right.DiDevice=0;
	if (Left.DiDevice>(NumberofJoysticks-1))
		Left.DiDevice=0;
	SetStickNumbers(Left.DiDevice,Right.DiDevice);
	if (NumberofJoysticks==0)	//Use Mouse input if no Joysticks present
	{
		if (Left.UseMouse==3)
			Left.UseMouse=1;
		if (Right.UseMouse==3)
			Right.UseMouse=1;
	}
	return;
}

void UpdateSoundBar (unsigned short Left,unsigned short Right)
{
	if (hDlgBar==NULL)
		return;
	SendDlgItemMessage(hDlgBar,IDC_PROGRESSLEFT,PBM_SETPOS,Left>>8,0);
	SendDlgItemMessage(hDlgBar,IDC_PROGRESSRIGHT,PBM_SETPOS,Right>>8,0);
	return;
}

void UpdateTapeCounter(unsigned int Counter,unsigned char TapeMode)
{
	if (hDlgTape==NULL)
		return;
	TapeCounter=Counter;
	Tmode=TapeMode;
	sprintf(OutBuffer,"%i",TapeCounter);
	SendDlgItemMessage(hDlgTape,IDC_TCOUNT,WM_SETTEXT,strlen(OutBuffer),(LPARAM)(LPCSTR)OutBuffer);
	SendDlgItemMessage(hDlgTape,IDC_MODE,WM_SETTEXT,strlen(Tmodes[Tmode]),(LPARAM)(LPCSTR)Tmodes[Tmode]);
	GetTapeName(TapeFileName);
	PathStripPath ( TapeFileName);  
	SendDlgItemMessage(hDlgTape,IDC_TAPEFILE,WM_SETTEXT,strlen(TapeFileName),(LPARAM)(LPCSTR)TapeFileName);

	switch (Tmode)
	{
	case REC:
		SendDlgItemMessage(hDlgTape,IDC_MODE,EM_SETBKGNDCOLOR ,0,(LPARAM)RGB(0xAF,0,0));
		break;

	case PLAY:
		SendDlgItemMessage(hDlgTape,IDC_MODE,EM_SETBKGNDCOLOR ,0,(LPARAM)RGB(0,0xAF,0));
		break;

	default:
		SendDlgItemMessage(hDlgTape,IDC_MODE,EM_SETBKGNDCOLOR ,0,(LPARAM)RGB(0,0,0));
		break;
	}
//		SendDlgItemMessage(hDlgTape,IDC_MODE,EM_SETCHARFORMAT ,SCF_ALL,(LPARAM)&CounterText); //&ModeText
	return;
}


LRESULT CALLBACK BitBanger(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG: //IDC_PRINTMON
			if (!strlen(SerialCaptureFile))
				strcpy(SerialCaptureFile,"No Capture File");
			SendDlgItemMessage(hDlg,IDC_SERIALFILE,WM_SETTEXT,strlen(SerialCaptureFile),(LPARAM)(LPCSTR)SerialCaptureFile);
			SendDlgItemMessage(hDlg,IDC_LF,BM_SETCHECK,TextMode,0);
			SendDlgItemMessage(hDlg,IDC_PRINTMON,BM_SETCHECK,PrtMon,0);
			break;

		case WM_COMMAND:

			switch (LOWORD(wParam))
			{
			case IDC_OPEN:
				SelectFile(SerialCaptureFile);
				SendDlgItemMessage(hDlg,IDC_SERIALFILE,WM_SETTEXT,strlen(SerialCaptureFile),(LPARAM)(LPCSTR)SerialCaptureFile);
				break;

			case IDC_CLOSE:
				ClosePrintFile();
				strcpy(SerialCaptureFile,"No Capture File");
				SendDlgItemMessage(hDlg,IDC_SERIALFILE,WM_SETTEXT,strlen(SerialCaptureFile),(LPARAM)(LPCSTR)SerialCaptureFile);
				PrtMon=FALSE;
				SetMonState(PrtMon);
				SendDlgItemMessage(hDlg,IDC_PRINTMON,BM_SETCHECK,PrtMon,0);
				break;

			case IDC_LF:
				TextMode=(char)SendDlgItemMessage(hDlg,IDC_LF,BM_GETCHECK,0,0);
				SetSerialParams(TextMode);
				break;

			case IDC_PRINTMON:
				PrtMon=(char)SendDlgItemMessage(hDlg,IDC_PRINTMON,BM_GETCHECK,0,0);
				SetMonState(PrtMon);

			}	//End switch wParam
			
			break;
	}
	return(0);
}

int SelectFile(char *FileName)
{
	OPENFILENAME ofn ;	
	char Dummy[MAX_PATH]="";
	char TempFileName[MAX_PATH]="";

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME);
	ofn.hwndOwner         = NULL;
	ofn.Flags             = OFN_HIDEREADONLY;
	ofn.hInstance         = GetModuleHandle(0);
	ofn. lpstrDefExt      = "txt";
	ofn.lpstrFilter       =	"Text File\0*.txt\0\0";	
	ofn.nFilterIndex      = 0 ;					// current filter index
	ofn.lpstrFile         = TempFileName;		// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;			// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;				// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH;			// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = Dummy;				// initial directory
	ofn.lpstrTitle        = "Open print capture file";		// title bar string

	if ( GetOpenFileName(&ofn) )
		if (!(OpenPrintFile(TempFileName)))
			MessageBox(0,"Can't Open File","Can't open the file specified.",0);
	strcpy(FileName,TempFileName);
	return(1);
}