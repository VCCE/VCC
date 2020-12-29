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

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <Richedit.h>
#include <iostream>

#include <direct.h>

#include "defines.h"
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
#include "shlobj.h"
#include "CommandLine.h"
//#include "logger.h"
#include <assert.h>
using namespace std;
//
// forward declarations
//
unsigned char XY2Disp(unsigned char, unsigned char);
void Disp2XY(unsigned char *, unsigned char *, unsigned char);

int SelectFile(char *);
void RefreshJoystickStatus(void);

#define MAXCARDS 12
LRESULT CALLBACK CpuConfig(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK AudioConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MiscConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DisplayConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InputConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK JoyStickConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TapeConfig(HWND , UINT , WPARAM , LPARAM );
LRESULT CALLBACK BitBanger(HWND , UINT , WPARAM , LPARAM );
LRESULT CALLBACK Paths(HWND, UINT, WPARAM, LPARAM);

//
//	global variables
//
static unsigned short int	Ramchoice[4]={IDC_128K,IDC_512K,IDC_2M,IDC_8M};
static unsigned  int	LeftJoystickEmulation[3] = { IDC_LEFTSTANDARD,IDC_LEFTTHIRES,IDC_LEFTCCMAX };
static unsigned int	RightJoystickEmulation[3] = { IDC_RIGHTSTANDARD,IDC_RIGHTTHRES,IDC_RIGHTCCMAX };
static unsigned short int	Cpuchoice[2]={IDC_6809,IDC_6309};
static unsigned short int	Monchoice[2]={IDC_COMPOSITE,IDC_RGB};
static unsigned short int   PaletteChoice[2] = { IDC_ORG_PALETTE,IDC_UPD_PALETTE };
static HICON CpuIcons[2],MonIcons[2],JoystickIcons[4];
static unsigned char temp=0,temp2=0;
static char IniFileName[]="Vcc.ini";
static char IniFilePath[MAX_PATH]="";
static char TapeFileName[MAX_PATH]="";
static char ExecDirectory[MAX_PATH]="";
static char SerialCaptureFile[MAX_PATH]="";
static char TextMode=1,PrtMon=0;;
static unsigned char NumberofJoysticks=0;
TCHAR AppDataPath[MAX_PATH];

char OutBuffer[MAX_PATH]="";
char AppName[MAX_LOADSTRING]="";
STRConfig CurrentConfig;
static STRConfig TempConfig;

extern SystemState EmuState;
extern char StickName[MAXSTICKS][STRLEN];

static unsigned int TapeCounter=0;
static unsigned char Tmode=STOP;
char Tmodes[4][10]={"STOP","PLAY","REC","STOP"};
static int NumberOfSoundCards=0;

static JoyStick TempLeft, TempRight;

static SndCardList SoundCards[MAXCARDS];
static HWND hDlgBar=NULL,hDlgTape=NULL;


CHARFORMAT CounterText;
CHARFORMAT ModeText;

/*****************************************************************************/
/*
	for displaying key name
*/
char * const keyNames[] = { "","ESC","1","2","3","4","5","6","7","8","9","0","-","=","BackSp","Tab","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","[","]","Bkslash",";","'","Comma",".","/","CapsLk","Shift","Ctrl","Alt","Space","Enter","Insert","Delete","Home","End","PgUp","PgDown","Left","Right","Up","Down","F1","F2" };

//					            0	1      2	3	4	5	6	7	8	9	10	11	12	13	14	    15    16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44        45  46  47      48  49   50       51     52     53    54      55      56       57       58     59    60     61       62     63      64   65     66   67	
//char * const cocoKeyNames[] = { "","@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","Up","Down","Left","Right","Space","0","1","2","3","4","5","6","7","8","9",":",";","Comma","-",".","/","Enter","Clear","Break","Alt","Ctrl","F1","F2","Shift" };


char * getKeyName(int x)
{
	return keyNames[x];
}

#define SCAN_TRANS_COUNT	84

unsigned char _TranslateDisp2Scan[SCAN_TRANS_COUNT];
unsigned char _TranslateScan2Disp[SCAN_TRANS_COUNT] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,32,38,20,33,35,40,36,24,30,31,42,43,55,52,16,34,19,21,22,23,25,26,27,45,46,0,51,44,41,39,18,37,17,29,28,47,48,49,51,0,53,54,50,66,67,0,0,0,0,0,0,0,0,0,0,58,64,60,0,62,0,63,0,59,65,61,56,57 };


#define TABS 8
static HWND g_hWndConfig[TABS]; // Moved this outside the initialization function so that other functions could access the window handles when necessary.

unsigned char TranslateDisp2Scan(int x)
{
	assert(x >= 0 && x < SCAN_TRANS_COUNT);

	return _TranslateDisp2Scan[x];
}

unsigned char TranslateScan2Disp(int x)
{
	assert(x >= 0 && x < SCAN_TRANS_COUNT);

	return _TranslateScan2Disp[x];
}

void buildTransDisp2ScanTable()
{
	for (int Index = 0; Index < SCAN_TRANS_COUNT; Index++)
	{
		for (int Index2 = SCAN_TRANS_COUNT - 1; Index2 >= 0; Index2--)
		{
			if (Index2 == _TranslateScan2Disp[Index])
			{
				_TranslateDisp2Scan[Index2] = (unsigned char)Index;
			}
		}
	}
	// ???
	_TranslateDisp2Scan[0] = 0;
	// ???
	// Left and Right Shift
	_TranslateDisp2Scan[51] = DIK_LSHIFT;

}

/*****************************************************************************/

void LoadConfig(SystemState *LCState)
{
	HANDLE hr=NULL;
	int lasterror;

	buildTransDisp2ScanTable();

	LoadString(NULL, IDS_APP_TITLE,AppName, MAX_LOADSTRING);
	GetModuleFileName(NULL,ExecDirectory,MAX_PATH);
	PathRemoveFileSpec(ExecDirectory);
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, AppDataPath))) 
		OutputDebugString(AppDataPath);
	strcpy(CurrentConfig.PathtoExe,ExecDirectory);

	strcat(AppDataPath, "\\VCC");
	if (_mkdir(AppDataPath) != 0) { OutputDebugString("Unable to create VCC config folder."); }
	
	if (*CmdArg.IniFile) {
		GetFullPathNameA(CmdArg.IniFile,MAX_PATH,IniFilePath,0);
	} else {
		strcpy(IniFilePath, AppDataPath);
		strcat(IniFilePath, "\\");
		strcat(IniFilePath, IniFileName);
	}

	LCState->ScanLines=0;
	NumberOfSoundCards=GetSoundCardList(SoundCards);
	ReadIniFile();
	CurrentConfig.RebootNow=0;
	UpdateConfig();
	RefreshJoystickStatus();
	SoundInit(EmuState.WindowHandle,SoundCards[CurrentConfig.SndOutDev].Guid,CurrentConfig.AudioRate);

//  Try to open the config file.  Create it if necessary.  Abort if failure.
	hr = CreateFile(IniFilePath,
					GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
					NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	lasterror = GetLastError();
	if (hr==INVALID_HANDLE_VALUE) { // Fatal could not open ini file
	    MessageBox(0,"Could not open ini file","Error",0);
		exit(0);
	} else {
		CloseHandle(hr);
		if (lasterror != ERROR_ALREADY_EXISTS) WriteIniFile();  //!=183
	}
}

unsigned char WriteIniFile(void)
{
	POINT tp = GetCurWindowSize();
	CurrentConfig.Resize = 1;
	GetCurrentModule(CurrentConfig.ModulePath);
	ValidatePath(CurrentConfig.ModulePath);
	ValidatePath(CurrentConfig.ExternalBasicImage);
	
	WritePrivateProfileString("Version","Release",AppName,IniFilePath);
	WritePrivateProfileInt("CPU","DoubleSpeedClock",CurrentConfig.CPUMultiplyer,IniFilePath);
	WritePrivateProfileInt("CPU","FrameSkip",CurrentConfig.FrameSkip,IniFilePath);
	WritePrivateProfileInt("CPU","Throttle",CurrentConfig.SpeedThrottle,IniFilePath);
	WritePrivateProfileInt("CPU","CpuType",CurrentConfig.CpuType,IniFilePath);
	WritePrivateProfileInt("CPU", "MaxOverClock", CurrentConfig.MaxOverclock, IniFilePath);

	WritePrivateProfileString("Audio","SndCard",CurrentConfig.SoundCardName,IniFilePath);
	WritePrivateProfileInt("Audio","Rate",CurrentConfig.AudioRate,IniFilePath);

	WritePrivateProfileInt("Video","MonitorType",CurrentConfig.MonitorType,IniFilePath);
	WritePrivateProfileInt("Video","PaletteType",CurrentConfig.PaletteType, IniFilePath);
	WritePrivateProfileInt("Video","ScanLines",CurrentConfig.ScanLines,IniFilePath);
	WritePrivateProfileInt("Video","AllowResize",CurrentConfig.Resize,IniFilePath);
	WritePrivateProfileInt("Video","ForceAspect",CurrentConfig.Aspect,IniFilePath);
	WritePrivateProfileInt("Video","RememberSize", CurrentConfig.RememberSize, IniFilePath);
	WritePrivateProfileInt("Video", "WindowSizeX", tp.x, IniFilePath);
	WritePrivateProfileInt("Video", "WindowSizeY", tp.y, IniFilePath);

	WritePrivateProfileInt("Memory","RamSize",CurrentConfig.RamSize,IniFilePath);
	WritePrivateProfileString("Memory", "ExternalBasicImage", CurrentConfig.ExternalBasicImage, IniFilePath);

	WritePrivateProfileInt("Misc","AutoStart",CurrentConfig.AutoStart,IniFilePath);
	WritePrivateProfileInt("Misc","CartAutoStart",CurrentConfig.CartAutoStart,IniFilePath);
	WritePrivateProfileInt("Misc","KeyMapIndex",CurrentConfig.KeyMap,IniFilePath);

	WritePrivateProfileString("Module", "OnBoot", CurrentConfig.ModulePath, IniFilePath);

	WritePrivateProfileInt("LeftJoyStick","UseMouse",Left.UseMouse,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Left",Left.Left,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Right",Left.Right,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Up",Left.Up,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Down",Left.Down,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Fire1",Left.Fire1,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Fire2",Left.Fire2,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","DiDevice",Left.DiDevice,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick", "HiResDevice", Left.HiRes, IniFilePath);
	WritePrivateProfileInt("RightJoyStick","UseMouse",Right.UseMouse,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Left",Right.Left,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Right",Right.Right,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Up",Right.Up,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Down",Right.Down,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Fire1",Right.Fire1,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Fire2",Right.Fire2,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","DiDevice",Right.DiDevice,IniFilePath);
	WritePrivateProfileInt("RightJoyStick", "HiResDevice", Right.HiRes, IniFilePath);

//  Flush inifile
	WritePrivateProfileString(NULL,NULL,NULL,IniFilePath);
	return(0);
}

unsigned char ReadIniFile(void)
{
	HANDLE hr=NULL;
	unsigned char Index=0;
	POINT p;
//	LPTSTR tmp;

	//Loads the config structure from the hard disk
	CurrentConfig.CPUMultiplyer = GetPrivateProfileInt("CPU","DoubleSpeedClock",2,IniFilePath);
	CurrentConfig.FrameSkip = GetPrivateProfileInt("CPU","FrameSkip",1,IniFilePath);
	CurrentConfig.SpeedThrottle = GetPrivateProfileInt("CPU","Throttle",1,IniFilePath);
	CurrentConfig.CpuType = GetPrivateProfileInt("CPU","CpuType",0,IniFilePath);
	CurrentConfig.MaxOverclock = GetPrivateProfileInt("CPU", "MaxOverClock",227, IniFilePath);

	CurrentConfig.AudioRate = GetPrivateProfileInt("Audio","Rate",3,IniFilePath);
	GetPrivateProfileString("Audio","SndCard","",CurrentConfig.SoundCardName,63,IniFilePath);

	CurrentConfig.MonitorType = GetPrivateProfileInt("Video","MonitorType",1,IniFilePath);
	CurrentConfig.PaletteType = GetPrivateProfileInt("Video", "PaletteType",1,IniFilePath);
	CurrentConfig.ScanLines = GetPrivateProfileInt("Video","ScanLines",0,IniFilePath);

	CurrentConfig.Resize = GetPrivateProfileInt("Video","AllowResize",0,IniFilePath);	
	CurrentConfig.Aspect = GetPrivateProfileInt("Video","ForceAspect",0,IniFilePath);
	CurrentConfig.RememberSize = GetPrivateProfileInt("Video","RememberSize",0,IniFilePath);
	CurrentConfig.WindowSizeX= GetPrivateProfileInt("Video", "WindowSizeX", 640, IniFilePath);
	CurrentConfig.WindowSizeY = GetPrivateProfileInt("Video", "WindowSizeY", 480, IniFilePath);
	CurrentConfig.AutoStart = GetPrivateProfileInt("Misc","AutoStart",1,IniFilePath);
	CurrentConfig.CartAutoStart = GetPrivateProfileInt("Misc","CartAutoStart",1,IniFilePath);


	CurrentConfig.RamSize = GetPrivateProfileInt("Memory","RamSize",1,IniFilePath);
	GetPrivateProfileString("Memory","ExternalBasicImage","",CurrentConfig.ExternalBasicImage,MAX_PATH,IniFilePath);

	GetPrivateProfileString("Module","OnBoot","",CurrentConfig.ModulePath,MAX_PATH,IniFilePath);
	
	CurrentConfig.KeyMap = GetPrivateProfileInt("Misc","KeyMapIndex",0,IniFilePath);
	if (CurrentConfig.KeyMap>3)
		CurrentConfig.KeyMap=0;	//Default to DECB Mapping
	vccKeyboardBuildRuntimeTable((keyboardlayout_e)CurrentConfig.KeyMap);

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
	Left.HiRes= GetPrivateProfileInt("LeftJoyStick", "HiResDevice", 0, IniFilePath);
	Right.UseMouse=GetPrivateProfileInt("RightJoyStick","UseMouse",1,IniFilePath);
	Right.Left=GetPrivateProfileInt("RightJoyStick","Left",75,IniFilePath);
	Right.Right=GetPrivateProfileInt("RightJoyStick","Right",77,IniFilePath);
	Right.Up=GetPrivateProfileInt("RightJoyStick","Up",72,IniFilePath);
	Right.Down=GetPrivateProfileInt("RightJoyStick","Down",80,IniFilePath);
	Right.Fire1=GetPrivateProfileInt("RightJoyStick","Fire1",59,IniFilePath);
	Right.Fire2=GetPrivateProfileInt("RightJoyStick","Fire2",60,IniFilePath);
	Right.DiDevice=GetPrivateProfileInt("RightJoyStick","DiDevice",0,IniFilePath);
	Right.HiRes = GetPrivateProfileInt("RightJoyStick", "HiResDevice", 0, IniFilePath);

	GetPrivateProfileString("DefaultPaths", "CassPath", "", CurrentConfig.CassPath, MAX_PATH, IniFilePath);
	GetPrivateProfileString("DefaultPaths", "FloppyPath", "", CurrentConfig.FloppyPath, MAX_PATH, IniFilePath);
	GetPrivateProfileString("DefaultPaths", "COCO3ROMPath", "", CurrentConfig.COCO3ROMPath, MAX_PATH, IniFilePath);

	for (Index = 0; Index < NumberOfSoundCards; Index++)
	{
		if (!strcmp(SoundCards[Index].CardName, CurrentConfig.SoundCardName))
		{
			CurrentConfig.SndOutDev = Index;
		}
	}

	TempConfig=CurrentConfig;
	InsertModule (CurrentConfig.ModulePath);	// Should this be here?
	CurrentConfig.Resize = 1; //Checkbox removed. Remove this from the ini? 
	if (CurrentConfig.RememberSize) {
		p.x = CurrentConfig.WindowSizeX;
		p.y = CurrentConfig.WindowSizeY;
		SetWindowSize(p);
	}
	else {
		p.x = 640;
		p.y = 480;
		SetWindowSize(p);
	}
	return(0);
}

char * BasicRomName(void)
{
	return(CurrentConfig.ExternalBasicImage); 
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char TabTitles[TABS][10]={"Audio","CPU","Display","Keyboard","Joysticks","Misc","Tape","BitBanger"};
	static unsigned char TabCount=0,SelectedTab=0;
	static HWND hWndTabDialog;
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

				vccKeyboardBuildRuntimeTable((keyboardlayout_e)CurrentConfig.KeyMap);

				Right=TempRight;
				Left=TempLeft;
				SetStickNumbers(Left.DiDevice,Right.DiDevice);

				for (temp = 0; temp < TABS; temp++)
				{
					DestroyWindow(g_hWndConfig[temp]);
				}
#ifdef CONFIG_DIALOG_MODAL
				EndDialog(hDlg, LOWORD(wParam));
#else
				DestroyWindow(hDlg);
#endif
				EmuState.ConfigDialog=NULL;
				break;

			case IDAPPLY:
				EmuState.ResetPending=4;	
				if ( (CurrentConfig.RamSize != TempConfig.RamSize) | (CurrentConfig.CpuType != TempConfig.CpuType) )
					EmuState.ResetPending=2;
				if ((CurrentConfig.SndOutDev != TempConfig.SndOutDev) | (CurrentConfig.AudioRate != TempConfig.AudioRate))
					SoundInit(EmuState.WindowHandle,SoundCards[TempConfig.SndOutDev].Guid,TempConfig.AudioRate);

				CurrentConfig=TempConfig;

				vccKeyboardBuildRuntimeTable((keyboardlayout_e)CurrentConfig.KeyMap);

				Right=TempRight;
				Left=TempLeft;
				SetStickNumbers(Left.DiDevice,Right.DiDevice);
			break;

			case IDCANCEL:
				for (temp = 0; temp < TABS; temp++)
				{
					DestroyWindow(g_hWndConfig[temp]);
				}
#ifdef CONFIG_DIALOG_MODAL
				EndDialog(hDlg, LOWORD(wParam));
#else
				DestroyWindow(hDlg);
#endif
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

//<EJJ>
void SetIniFilePath( char *Path)
{
    //  Path must be to an existing ini file
    strcpy(IniFilePath,Path);
}

char * AppDirectory() 
{
    // This only works after LoadConfig has been called
	return AppDataPath;
}
//<EJJ/>

void UpdateConfig (void)
{
	SetPaletteType();
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

/**
 * Increase the overclock speed, as seen after a POKE 65497,0.
 * Valid values are [2,100].
 */
void IncreaseOverclockSpeed()
{
	if (TempConfig.CPUMultiplyer >= CurrentConfig.MaxOverclock)
	{
		return;
	}

	TempConfig.CPUMultiplyer = (unsigned char)(TempConfig.CPUMultiplyer + 1);

	// Send updates to the dialog if it's open.
	if (EmuState.ConfigDialog != NULL)
	{
		HWND hDlg = g_hWndConfig[1];
		SendDlgItemMessage(hDlg, IDC_CLOCKSPEED, TBM_SETPOS, TRUE, TempConfig.CPUMultiplyer);
		sprintf(OutBuffer, "%2.3f Mhz", (float)TempConfig.CPUMultiplyer * 0.894);
		SendDlgItemMessage(hDlg, IDC_CLOCKDISPLAY, WM_SETTEXT, strlen(OutBuffer), (LPARAM)(LPCSTR)OutBuffer);
	}

	CurrentConfig = TempConfig;
	EmuState.ResetPending = 4; // Without this, changing the config does nothing.
}

/**
 * Decrease the overclock speed, as seen after a POKE 65497,0.
 *
 * Setting this value to 0 will make the emulator pause.  Hence the minimum of 2.
 */
void DecreaseOverclockSpeed()
{
	if (TempConfig.CPUMultiplyer == 2)
	{
		return;
	}

	TempConfig.CPUMultiplyer = (unsigned char)(TempConfig.CPUMultiplyer - 1);

	// Send updates to the dialog if it's open.
	if (EmuState.ConfigDialog != NULL)
	{
		HWND hDlg = g_hWndConfig[1];
		SendDlgItemMessage(hDlg, IDC_CLOCKSPEED, TBM_SETPOS, TRUE, TempConfig.CPUMultiplyer);
		sprintf(OutBuffer, "%2.3f Mhz", (float)TempConfig.CPUMultiplyer * 0.894);
		SendDlgItemMessage(hDlg, IDC_CLOCKDISPLAY, WM_SETTEXT, strlen(OutBuffer), (LPARAM)(LPCSTR)OutBuffer);
	}

	CurrentConfig = TempConfig;
	EmuState.ResetPending = 4;
}

LRESULT CALLBACK CpuConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_CLOCKSPEED,TBM_SETRANGE,TRUE,MAKELONG(2,CurrentConfig.MaxOverclock ));	//Maximum overclock
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
	static bool isRGB;
	switch (message)
	{
		case WM_INITDIALOG:

			SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_SETRANGE,TRUE,MAKELONG(1,6) );
			SendDlgItemMessage(hDlg,IDC_SCANLINES,BM_SETCHECK,TempConfig.ScanLines,0);
			SendDlgItemMessage(hDlg,IDC_THROTTLE,BM_SETCHECK,TempConfig.SpeedThrottle,0);
			SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_SETPOS,TRUE,TempConfig.FrameSkip);
			SendDlgItemMessage(hDlg,IDC_RESIZE,BM_SETCHECK,TempConfig.Resize,0);
			SendDlgItemMessage(hDlg,IDC_ASPECT,BM_SETCHECK,TempConfig.Aspect,0);
			SendDlgItemMessage(hDlg, IDC_REMEMBER_SIZE, BM_SETCHECK, TempConfig.RememberSize, 0);
			sprintf(OutBuffer,"%i",TempConfig.FrameSkip);
			SendDlgItemMessage(hDlg,IDC_FRAMEDISPLAY,WM_SETTEXT,strlen(OutBuffer),(LPARAM)(LPCSTR)OutBuffer);
			for (temp=0;temp<=1;temp++)
				SendDlgItemMessage(hDlg,Monchoice[temp],BM_SETCHECK,(temp==TempConfig.MonitorType),0);
			if (TempConfig.MonitorType == 1) { //If RGB monitor is chosen, gray out palette choice
				isRGB = TRUE;
				SendDlgItemMessage(hDlg, IDC_ORG_PALETTE, BM_SETSTATE, 1, 0);
				SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETSTATE, 1, 0);
				SendDlgItemMessage(hDlg, IDC_ORG_PALETTE, BM_SETDONTCLICK, 1, 0);
				SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETDONTCLICK, 1, 0);
			
			}
			SendDlgItemMessage(hDlg,IDC_MONTYPE,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)MonIcons[TempConfig.MonitorType]);
			for (temp = 0; temp <= 1; temp++)
				SendDlgItemMessage(hDlg, PaletteChoice[temp], BM_SETCHECK, (temp == TempConfig.PaletteType), 0);
			
		break;

		case WM_HSCROLL:
			TempConfig.FrameSkip=(unsigned char) SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_GETPOS,(WPARAM) 0, (WPARAM) 0);
			sprintf(OutBuffer,"%i",TempConfig.FrameSkip);
			SendDlgItemMessage(hDlg,IDC_FRAMEDISPLAY,WM_SETTEXT,strlen(OutBuffer),(LPARAM)(LPCSTR)OutBuffer);
		break;
		
		case WM_COMMAND:
			TempConfig.Resize = 1; //(unsigned char)SendDlgItemMessage(hDlg,IDC_RESIZE,BM_GETCHECK,0,0);
			TempConfig.Aspect = (unsigned char)SendDlgItemMessage(hDlg,IDC_ASPECT,BM_GETCHECK,0,0);
			TempConfig.ScanLines  = (unsigned char)SendDlgItemMessage(hDlg,IDC_SCANLINES,BM_GETCHECK,0,0);
			TempConfig.SpeedThrottle = (unsigned char)SendDlgItemMessage(hDlg,IDC_THROTTLE,BM_GETCHECK,0,0);
			TempConfig.RememberSize = (unsigned char)SendDlgItemMessage(hDlg, IDC_REMEMBER_SIZE, BM_GETCHECK, 0, 0);
			//POINT p = { 640,480 };
			switch (LOWORD (wParam))
			{
				
			case IDC_REMEMBER_SIZE:
				TempConfig.Resize = 1;
				SendDlgItemMessage(hDlg, IDC_RESIZE, BM_GETCHECK, 1, 0);
				break;

			case IDC_COMPOSITE:
					isRGB = FALSE;
					for (temp = 0; temp <= 1; temp++) //This finds the current Monitor choice, then sets both buttons in the nested loop.
					if (LOWORD(wParam) == Monchoice[temp])
					{
						for (temp2 = 0; temp2 <= 1; temp2++)
							SendDlgItemMessage(hDlg, Monchoice[temp2], BM_SETCHECK, 0, 0);
							SendDlgItemMessage(hDlg, Monchoice[temp], BM_SETCHECK, 1, 0);
						TempConfig.MonitorType = temp;
					}
					SendDlgItemMessage(hDlg, IDC_MONTYPE, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)MonIcons[TempConfig.MonitorType]);
					SendDlgItemMessage(hDlg, IDC_ORG_PALETTE, BM_SETSTATE, 0, 0);
					SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETSTATE, 0, 0);
					break;
				case IDC_RGB:
					isRGB = TRUE;
					for (temp=0;temp<=1;temp++) //This finds the current Monitor choice, then sets both buttons in the nested loop.
						if (LOWORD(wParam)==Monchoice[temp])
						{
							for (temp2=0;temp2<=1;temp2++)
								SendDlgItemMessage(hDlg,Monchoice[temp2],BM_SETCHECK,0,0);
								SendDlgItemMessage(hDlg,Monchoice[temp],BM_SETCHECK,1,0);
								TempConfig.MonitorType=temp;
						}
					SendDlgItemMessage(hDlg,IDC_MONTYPE,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)MonIcons[TempConfig.MonitorType]);
					//If RGB is chosen, disable palette buttons.
					SendDlgItemMessage(hDlg, IDC_ORG_PALETTE, BM_SETSTATE, 1, 0);
					SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETSTATE, 1, 0);
					SendDlgItemMessage(hDlg, IDC_ORG_PALETTE, BM_SETDONTCLICK, 1, 0);
					SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETDONTCLICK, 1, 0);
					break;
				case IDC_ORG_PALETTE: 
					if (!isRGB) {
						//Original Composite palette
						SendDlgItemMessage(hDlg, IDC_ORG_PALETTE, BM_SETCHECK, 1, 0);
						SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETCHECK, 0, 0);
						TempConfig.PaletteType = 0;
					}
					else { SendDlgItemMessage(hDlg, IDC_ORG_PALETTE, BM_SETSTATE, 1, 0); }
					break;
				case IDC_UPD_PALETTE:
					if (!isRGB) {
						//New Composite palette
						SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETCHECK, 1, 0);
						SendDlgItemMessage(hDlg, IDC_ORG_PALETTE, BM_SETCHECK, 0, 0);
						TempConfig.PaletteType = 1;
					}
					else { SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETSTATE, 1, 0); }
				break;

			}	//End switch LOWORD(wParam)
		break;	//break WM_COMMAND
	}			//END switch Message
	return(0);
}

LRESULT CALLBACK InputConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			// copy keyboard layout names to the pull-down menu
			for (int x = 0; x <kKBLayoutCount; x++)
			{
				SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_ADDSTRING,(WPARAM)0,(LPARAM)k_keyboardLayoutNames[x]);
			}
			// select the current layout
			SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_SETCURSEL,(WPARAM)CurrentConfig.KeyMap,(LPARAM)0);
		break;

		case WM_COMMAND:
			TempConfig.KeyMap = (unsigned char)SendDlgItemMessage(hDlg,IDC_KBCONFIG,CB_GETCURSEL,0,0);
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
			for (temp = 0; temp < 68; temp++)
			{
				for (temp2 = 0; temp2 < 6; temp2++)
				{
					SendDlgItemMessage(hDlg, LeftJoyStick[temp2], CB_ADDSTRING, (WPARAM)0, (LPARAM)getKeyName(temp));
					SendDlgItemMessage(hDlg, RightJoyStick[temp2], CB_ADDSTRING, (WPARAM)0, (LPARAM)getKeyName(temp));
				}
			}

			for (temp = 0; temp < 6; temp++)
			{
				EnableWindow(GetDlgItem(hDlg, LeftJoyStick[temp]), (Left.UseMouse == 0));
			}
			for (temp = 0; temp < 6; temp++)
			{
				EnableWindow(GetDlgItem(hDlg, RightJoyStick[temp]), (Right.UseMouse == 0));
			}

			for (temp=0;temp<=2;temp++)
			{
				SendDlgItemMessage(hDlg, LeftJoystickEmulation[temp], BM_SETCHECK, (temp == Left.HiRes), 0);
				SendDlgItemMessage(hDlg, RightJoystickEmulation[temp], BM_SETCHECK, (temp == Right.HiRes), 0);
			}

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

			SendDlgItemMessage(hDlg,LeftJoyStick[0],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Left.Left),(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[1],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Left.Right),(LPARAM)0);	
			SendDlgItemMessage(hDlg,LeftJoyStick[2],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Left.Up),(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[3],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Left.Down),(LPARAM)0);	
			SendDlgItemMessage(hDlg,LeftJoyStick[4],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Left.Fire1),(LPARAM)0);	
			SendDlgItemMessage(hDlg,LeftJoyStick[5],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Left.Fire2),(LPARAM)0);	
			SendDlgItemMessage(hDlg,RightJoyStick[0],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Right.Left),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[1],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Right.Right),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[2],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Right.Up),(LPARAM)0);	
			SendDlgItemMessage(hDlg,RightJoyStick[3],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Right.Down),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[4],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Right.Fire1),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[5],CB_SETCURSEL,(WPARAM)TranslateScan2Disp(Right.Fire2),(LPARAM)0);

			for (temp = 0; temp <= 3; temp++)
			{
				SendDlgItemMessage(hDlg, LeftRadios[temp], BM_SETCHECK, temp == Left.UseMouse, 0);
			}
			for (temp = 0; temp <= 3; temp++)
			{
				SendDlgItemMessage(hDlg, RightRadios[temp], BM_SETCHECK, temp == Right.UseMouse, 0);
			}
			TempLeft = Left;
			TempRight=Right;
			SendDlgItemMessage(hDlg,IDC_LEFTICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[Left.UseMouse]);
			SendDlgItemMessage(hDlg,IDC_RIGHTICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[Right.UseMouse]);
		break;

		case WM_COMMAND:
			for (temp = 0; temp <= 3; temp++)
			{
				if (LOWORD(wParam) == LeftRadios[temp])
				{
					for (temp2 = 0; temp2 <= 3; temp2++)
						SendDlgItemMessage(hDlg, LeftRadios[temp2], BM_SETCHECK, 0, 0);
					SendDlgItemMessage(hDlg, LeftRadios[temp], BM_SETCHECK, 1, 0);
					TempLeft.UseMouse = temp;
				}
			}

			for (temp = 0; temp <= 3; temp++)
			{
				if (LOWORD(wParam) == RightRadios[temp])
				{
					for (temp2 = 0; temp2 <= 3; temp2++)
						SendDlgItemMessage(hDlg, RightRadios[temp2], BM_SETCHECK, 0, 0);
					SendDlgItemMessage(hDlg, RightRadios[temp], BM_SETCHECK, 1, 0);
					TempRight.UseMouse = temp;
				}
			}

			for (temp = 0; temp <= 2; temp++)
			{
				if (LOWORD(wParam) == LeftJoystickEmulation[temp])
				{
					for (temp2 = 0; temp2 <= 2; temp2++)
						SendDlgItemMessage(hDlg, LeftJoystickEmulation[temp2], BM_SETCHECK, 0, 0);
					SendDlgItemMessage(hDlg, LeftJoystickEmulation[temp], BM_SETCHECK, 1, 0);
					TempLeft.HiRes = temp;
				}
			}

			for (temp = 0; temp <= 2; temp++)
			{
				if (LOWORD(wParam) == RightJoystickEmulation[temp])
				{
					for (temp2 = 0; temp2 <= 2; temp2++)
						SendDlgItemMessage(hDlg, RightJoystickEmulation[temp2], BM_SETCHECK, 0, 0);
					SendDlgItemMessage(hDlg, RightJoystickEmulation[temp], BM_SETCHECK, 1, 0);
					TempRight.HiRes = temp;
				}
			}

			for (temp = 0; temp < 6; temp++)
			{
				EnableWindow(GetDlgItem(hDlg, LeftJoyStick[temp]), (TempLeft.UseMouse == 0));
			}

			for (temp = 0; temp < 6; temp++)
			{
				EnableWindow(GetDlgItem(hDlg, RightJoyStick[temp]), (TempRight.UseMouse == 0));
			}

			EnableWindow( GetDlgItem(hDlg,IDC_LEFTAUDIODEVICE),(TempLeft.UseMouse==2));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTAUDIODEVICE),(TempRight.UseMouse==2));
			EnableWindow( GetDlgItem(hDlg,IDC_LEFTJOYSTICKDEVICE),(TempLeft.UseMouse==3));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTJOYSTICKDEVICE),(TempRight.UseMouse==3));

			TempLeft.Left=TranslateDisp2Scan(SendDlgItemMessage(hDlg,LeftJoyStick[0],CB_GETCURSEL,0,0));
			TempLeft.Right=TranslateDisp2Scan(SendDlgItemMessage(hDlg,LeftJoyStick[1],CB_GETCURSEL,0,0));
			TempLeft.Up=TranslateDisp2Scan(SendDlgItemMessage(hDlg,LeftJoyStick[2],CB_GETCURSEL,0,0));
			TempLeft.Down=TranslateDisp2Scan(SendDlgItemMessage(hDlg,LeftJoyStick[3],CB_GETCURSEL,0,0));
			TempLeft.Fire1=TranslateDisp2Scan(SendDlgItemMessage(hDlg,LeftJoyStick[4],CB_GETCURSEL,0,0));
			TempLeft.Fire2=TranslateDisp2Scan(SendDlgItemMessage(hDlg,LeftJoyStick[5],CB_GETCURSEL,0,0));

			TempRight.Left=TranslateDisp2Scan(SendDlgItemMessage(hDlg,RightJoyStick[0],CB_GETCURSEL,0,0));
			TempRight.Right=TranslateDisp2Scan(SendDlgItemMessage(hDlg,RightJoyStick[1],CB_GETCURSEL,0,0));
			TempRight.Up=TranslateDisp2Scan(SendDlgItemMessage(hDlg,RightJoyStick[2],CB_GETCURSEL,0,0));
			TempRight.Down=TranslateDisp2Scan(SendDlgItemMessage(hDlg,RightJoyStick[3],CB_GETCURSEL,0,0));
			TempRight.Fire1=TranslateDisp2Scan(SendDlgItemMessage(hDlg,RightJoyStick[4],CB_GETCURSEL,0,0));
			TempRight.Fire2=TranslateDisp2Scan(SendDlgItemMessage(hDlg,RightJoyStick[5],CB_GETCURSEL,0,0));
			
			TempRight.DiDevice=(unsigned char)SendDlgItemMessage(hDlg,IDC_RIGHTJOYSTICKDEVICE,CB_GETCURSEL,0,0);
			TempLeft.DiDevice=(unsigned char)SendDlgItemMessage(hDlg,IDC_LEFTJOYSTICKDEVICE,CB_GETCURSEL,0,0);	//Fix Me;

			SendDlgItemMessage(hDlg,IDC_LEFTICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[TempLeft.UseMouse]);
			SendDlgItemMessage(hDlg,IDC_RIGHTICON,STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[TempRight.UseMouse]);
		break;
	} //END switch message
	return(0);
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
LRESULT CALLBACK Paths(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	return 1;

}

int SelectFile(char *FileName)
{
	OPENFILENAME ofn ;	
	char Dummy[MAX_PATH]="";
	char TempFileName[MAX_PATH]="";
	char CapFilePath[MAX_PATH];
	GetPrivateProfileString("DefaultPaths", "CapFilePath", "", CapFilePath, MAX_PATH, IniFilePath);

	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize       = sizeof (OPENFILENAME);
	ofn.hwndOwner = EmuState.WindowHandle; // GetTopWindow(NULL);
	ofn.Flags             = OFN_HIDEREADONLY;
	ofn.hInstance         = GetModuleHandle(0);
	ofn. lpstrDefExt      = "txt";
	ofn.lpstrFilter       =	"Text File\0*.txt\0\0";	
	ofn.nFilterIndex      = 0 ;					// current filter index
	ofn.lpstrFile         = TempFileName;		// contains full path and filename on return
	ofn.nMaxFile          = MAX_PATH;			// sizeof lpstrFile
	ofn.lpstrFileTitle    = NULL;				// filename and extension only
	ofn.nMaxFileTitle     = MAX_PATH;			// sizeof lpstrFileTitle
	ofn.lpstrInitialDir   = CapFilePath;				// initial directory
	ofn.lpstrTitle        = "Open print capture file";		// title bar string

	if (GetOpenFileName(&ofn)) {
		if (!(OpenPrintFile(TempFileName))) {
			MessageBox(0, "Can't Open File", "Can't open the file specified.", 0);
		}
		string tmp = ofn.lpstrFile;
		int idx;
		idx = tmp.find_last_of("\\");
		tmp = tmp.substr(0, idx);
		strcpy(CapFilePath, tmp.c_str());
		if (CapFilePath != "") {
			WritePrivateProfileString("DefaultPaths", "CapFilePath", CapFilePath, IniFilePath);
		}
	}
	strcpy(FileName,TempFileName);
	
	
	
	return(1);
}

void SetWindowSize(POINT p) {
	int width = p.x+16;
	int height = p.y+81;
	HWND handle = GetActiveWindow();
	::SetWindowPos(handle, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}
int GetKeyboardLayout() {
	return(CurrentConfig.KeyMap);
}

int GetPaletteType() {
	return(CurrentConfig.PaletteType);
}

int GetRememberSize() {
	return((int) CurrentConfig.RememberSize);
	//return(1);
}

POINT GetIniWindowSize() {
	POINT out;
	
	out.x = CurrentConfig.WindowSizeX;
	out.y = CurrentConfig.WindowSizeY;
	return(out);
}

