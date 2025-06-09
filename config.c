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
#include "DirectDrawInterface.h"
#include "joystickinput.h"
#include "keyboard.h"
#include "keyboardEdit.h"
#include "fileops.h"
#include "Cassette.h"
#pragma warning(disable:4091)
#include "shlobj.h"
#pragma warning(default:4091)
#include "CommandLine.h"
#include "logger.h"
#include <assert.h>

using namespace std;
using namespace VCC;

/********************************************/
/*        Local Function Templates          */
/********************************************/

int SelectFile(char *);
void RefreshJoystickStatus(void);
unsigned char TranslateDisp2Scan(int);
unsigned char TranslateScan2Disp(int);
void buildTransDisp2ScanTable();

LRESULT CALLBACK CpuConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AudioConfig(HWND, UINT, WPARAM, LPARAM);
//LRESULT CALLBACK MiscConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DisplayConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK InputConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK JoyStickConfig(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK TapeConfig(HWND , UINT , WPARAM , LPARAM );
LRESULT CALLBACK BitBanger(HWND , UINT , WPARAM , LPARAM );
LRESULT CALLBACK Paths(HWND, UINT, WPARAM, LPARAM);

/********************************************/
/*            Local Globals                 */
/********************************************/

// Structure for some (but not all) Vcc settings
typedef struct  {
	unsigned char	CPUMultiplyer;
	unsigned short	MaxOverclock;
	unsigned char	FrameSkip;
	unsigned char	SpeedThrottle;
	unsigned char	CpuType;
	unsigned char	HaltOpcEnabled;   // 0x15   halt enabled
	unsigned char	BreakOpcEnabled;  // 0x113E halt enabled
//	unsigned char	AudioMute;
	unsigned char	MonitorType;
	unsigned char   PaletteType;
	unsigned char	ScanLines;
	unsigned char	Resize;
	unsigned char	Aspect;
	unsigned char	RememberSize;
	Rect			WindowRect;
	unsigned char	RamSize;
	unsigned char	AutoStart;
	unsigned char	CartAutoStart;
	unsigned char	RebootNow;
	unsigned char	SndOutDev;
	unsigned char	KeyMap;
	char			SoundCardName[64];
	unsigned int	AudioRate;	// 0 = Mute
	char			ModulePath[MAX_PATH];
	char			PathtoExe[MAX_PATH];
	char			FloppyPath[MAX_PATH];
	char			CassPath[MAX_PATH];
    unsigned char   ShowMousePointer;
	unsigned char	UseExtCocoRom;
	char        	ExtRomFile[MAX_PATH];
	unsigned char   EnableOverclock;
} STRConfig;

static STRConfig CurrentConfig;

static HICON CpuIcons[2],MonIcons[2],JoystickIcons[4];
static unsigned char temp=0,temp2=0;
static char IniFileName[]="Vcc.ini";
static char IniFilePath[MAX_PATH]="";
static char KeyMapFilePath[MAX_PATH]="";
static char TapeFileName[MAX_PATH]="";
static char ExecDirectory[MAX_PATH]="";
static char SerialCaptureFile[MAX_PATH]="";
static unsigned char NumberofJoysticks=0;
TCHAR AppDataPath[MAX_PATH];

char OutBuffer[MAX_PATH]="";
char AppName[MAX_LOADSTRING]="";

extern SystemState EmuState;
extern char StickName[MAXSTICKS][STRLEN];

static unsigned int TapeCounter=0;
static unsigned char Tmode=STOP;
unsigned char TapeFastLoad = 1;
char Tmodes[4][10]={"STOP","PLAY","REC","STOP"};
static int NumberOfSoundCards=0;

#define MAXSOUNDCARDS 12
static SndCardList SoundCards[MAXSOUNDCARDS];

CHARFORMAT CounterText;
CHARFORMAT ModeText;

// Key names and translation tables for keyboard Joystick
#define KEYNAME(x) (keyNames[x])
const char * const keyNames[] = {
		"","ESC","1","2","3","4","5","6","7","8","9","0","-","=","BackSp",
		"Tab","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
		"P","Q","R","S","T","U","V","W","X","Y","Z","[","]","Bkslash",";",
		"'","Comma",".","/","CapsLk","Shift","Ctrl","Alt","Space","Enter",
		"Insert","Delete","Home","End","PgUp","PgDown","Left","Right",
		"Up","Down","F1","F2"};
#define SCAN_TRANS_COUNT 84
unsigned char _TranslateDisp2Scan[SCAN_TRANS_COUNT];
unsigned char _TranslateScan2Disp[SCAN_TRANS_COUNT] = {
		0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,32,38,20,33,35,40,36,24,30,
		31,42,43,55,52,16,34,19,21,22,23,25,26,27,45,46,0,51,44,41,39,18,
		37,17,29,28,47,48,49,51,0,53,54,50,66,67,0,0,0,0,0,0,0,0,0,0,58,
		64,60,0,62,0,63,0,59,65,61,56,57 };

/***********************************************************/
/*         Establish ini file and Load Settings            */
/***********************************************************/
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

	if (_mkdir(AppDataPath) != 0) {
		OutputDebugString("Unable to create VCC config folder.");
	}

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
	if (EmuState.WindowHandle != NULL)
		InitSound();

//  Try to open the config file.  Create it if necessary.  Abort if failure.
	hr = CreateFile(IniFilePath, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	lasterror = GetLastError();
	if (hr==INVALID_HANDLE_VALUE) { // Fatal could not open ini file
	    MessageBox(0,"Could not open ini file","Error",0);
		exit(0);
	} else {
		CloseHandle(hr);
		if (lasterror != ERROR_ALREADY_EXISTS) WriteIniFile();  //!=183
	}
}

void InitSound()
{
	SoundInit(EmuState.WindowHandle,
		SoundCards[CurrentConfig.SndOutDev].Guid, CurrentConfig.AudioRate);
}

/***********************************************************/
/*        Save Configuration Settings to ini file          */
/***********************************************************/
unsigned char WriteIniFile(void)
{
	Rect winRect = GetCurWindowSize();
	CurrentConfig.Resize = 1;      // How to restore default window size?

	// Prevent bad window size being written to the inifile
	if (winRect.w < 20 || winRect.h < 20) 
	{
		winRect.w = 640;
		winRect.h = 480;
	}

	GetCurrentModule(CurrentConfig.ModulePath);
	ValidatePath(CurrentConfig.ModulePath);

	WritePrivateProfileString("Version","Release",AppName,IniFilePath);
	WritePrivateProfileInt("CPU","DoubleSpeedClock",CurrentConfig.CPUMultiplyer,IniFilePath);
	WritePrivateProfileInt("CPU","FrameSkip",CurrentConfig.FrameSkip,IniFilePath);
	WritePrivateProfileInt("CPU","Throttle",CurrentConfig.SpeedThrottle,IniFilePath);
	WritePrivateProfileInt("CPU","CpuType",CurrentConfig.CpuType,IniFilePath);
	WritePrivateProfileInt("CPU", "MaxOverClock", CurrentConfig.MaxOverclock, IniFilePath);
	WritePrivateProfileInt("CPU","BreakEnabled",CurrentConfig.BreakOpcEnabled,IniFilePath);

	WritePrivateProfileString("Audio","SndCard",CurrentConfig.SoundCardName,IniFilePath);
	WritePrivateProfileInt("Audio","Rate",CurrentConfig.AudioRate,IniFilePath);

	WritePrivateProfileInt("Video","MonitorType",CurrentConfig.MonitorType,IniFilePath);
	WritePrivateProfileInt("Video","PaletteType",CurrentConfig.PaletteType, IniFilePath);
	WritePrivateProfileInt("Video","ScanLines",CurrentConfig.ScanLines,IniFilePath);
	WritePrivateProfileInt("Video","ForceAspect",CurrentConfig.Aspect,IniFilePath);
	WritePrivateProfileInt("Video","RememberSize", CurrentConfig.RememberSize, IniFilePath);
	WritePrivateProfileInt("Video", "WindowSizeX", winRect.w, IniFilePath);
	WritePrivateProfileInt("Video", "WindowSizeY", winRect.h, IniFilePath);
	WritePrivateProfileInt("Video", "WindowPosX", winRect.x, IniFilePath);
	WritePrivateProfileInt("Video", "WindowPosY", winRect.y, IniFilePath);

	WritePrivateProfileInt("Memory","RamSize",CurrentConfig.RamSize,IniFilePath);

	WritePrivateProfileInt("Misc","AutoStart",CurrentConfig.AutoStart,IniFilePath);
	WritePrivateProfileInt("Misc","CartAutoStart",CurrentConfig.CartAutoStart,IniFilePath);
	WritePrivateProfileInt("Misc","ShowMousePointer",CurrentConfig.ShowMousePointer,IniFilePath);
	WritePrivateProfileInt("Misc","KeyMapIndex",CurrentConfig.KeyMap,IniFilePath);
	WritePrivateProfileInt("Misc", "UseExtCocoRom", CurrentConfig.UseExtCocoRom, IniFilePath);
	WritePrivateProfileInt("Misc", "Overclock", CurrentConfig.EnableOverclock, IniFilePath);
	WritePrivateProfileString("Misc", "ExternalBasicImage", CurrentConfig.ExtRomFile,IniFilePath);

	WritePrivateProfileString("Module", "OnBoot", CurrentConfig.ModulePath, IniFilePath);

	WritePrivateProfileInt("LeftJoyStick","UseMouse",LeftJS.UseMouse,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Left",LeftJS.Left,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Right",LeftJS.Right,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Up",LeftJS.Up,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Down",LeftJS.Down,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Fire1",LeftJS.Fire1,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","Fire2",LeftJS.Fire2,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick","DiDevice",LeftJS.DiDevice,IniFilePath);
	WritePrivateProfileInt("LeftJoyStick", "HiResDevice", LeftJS.HiRes, IniFilePath);
	WritePrivateProfileInt("RightJoyStick","UseMouse",RightJS.UseMouse,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Left",RightJS.Left,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Right",RightJS.Right,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Up",RightJS.Up,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Down",RightJS.Down,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Fire1",RightJS.Fire1,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","Fire2",RightJS.Fire2,IniFilePath);
	WritePrivateProfileInt("RightJoyStick","DiDevice",RightJS.DiDevice,IniFilePath);
	WritePrivateProfileInt("RightJoyStick", "HiResDevice", RightJS.HiRes, IniFilePath);

    // Force flush inifile
	WritePrivateProfileString(NULL,NULL,NULL,IniFilePath);
	return(0);
}

/***********************************************************/
/*        Load Configuration Settings from ini file        */
/***********************************************************/
unsigned char ReadIniFile(void)
{
	HANDLE hr=NULL;
	unsigned char Index=0;

	//Loads the config structure from the hard disk
	CurrentConfig.CPUMultiplyer = GetPrivateProfileInt("CPU","DoubleSpeedClock",2,IniFilePath);
	CurrentConfig.FrameSkip = GetPrivateProfileInt("CPU","FrameSkip",1,IniFilePath);
	CurrentConfig.SpeedThrottle = GetPrivateProfileInt("CPU","Throttle",1,IniFilePath);
	CurrentConfig.CpuType = GetPrivateProfileInt("CPU","CpuType",0,IniFilePath);
	CurrentConfig.MaxOverclock = GetPrivateProfileInt("CPU","MaxOverClock",100,IniFilePath);
	CurrentConfig.BreakOpcEnabled = GetPrivateProfileInt("CPU","BreakEnabled",0,IniFilePath);

	CurrentConfig.AudioRate = GetPrivateProfileInt("Audio","Rate",3,IniFilePath);
	GetPrivateProfileString("Audio","SndCard","",CurrentConfig.SoundCardName,63,IniFilePath);

	CurrentConfig.MonitorType = GetPrivateProfileInt("Video","MonitorType",1,IniFilePath);
	CurrentConfig.PaletteType = GetPrivateProfileInt("Video","PaletteType",PALETTE_NTSC,IniFilePath);
	CurrentConfig.ScanLines = GetPrivateProfileInt("Video","ScanLines",0,IniFilePath);

	//CurrentConfig.Resize = GetPrivateProfileInt("Video","AllowResize",0,IniFilePath);
	CurrentConfig.Aspect = GetPrivateProfileInt("Video","ForceAspect",1,IniFilePath);
	CurrentConfig.RememberSize = GetPrivateProfileInt("Video","RememberSize",1,IniFilePath);
	CurrentConfig.WindowRect.w = GetPrivateProfileInt("Video", "WindowSizeX", 640, IniFilePath);
	CurrentConfig.WindowRect.h = GetPrivateProfileInt("Video", "WindowSizeY", 480, IniFilePath);
	CurrentConfig.WindowRect.x = GetPrivateProfileInt("Video", "WindowPosX", CW_USEDEFAULT, IniFilePath);
	CurrentConfig.WindowRect.y = GetPrivateProfileInt("Video", "WindowPosY", CW_USEDEFAULT, IniFilePath);
	CurrentConfig.AutoStart = GetPrivateProfileInt("Misc","AutoStart",1,IniFilePath);
	CurrentConfig.CartAutoStart = GetPrivateProfileInt("Misc","CartAutoStart",1,IniFilePath);
	CurrentConfig.ShowMousePointer = GetPrivateProfileInt("Misc","ShowMousePointer",1,IniFilePath);
	CurrentConfig.UseExtCocoRom=GetPrivateProfileInt("Misc","UseExtCocoRom",0,IniFilePath);
	CurrentConfig.EnableOverclock=GetPrivateProfileInt("Misc","Overclock",1,IniFilePath);
	GetPrivateProfileString("Misc","ExternalBasicImage","",CurrentConfig.ExtRomFile,MAX_PATH,IniFilePath);

	CurrentConfig.RamSize = GetPrivateProfileInt("Memory","RamSize",1,IniFilePath);

	GetPrivateProfileString("Module","OnBoot","",CurrentConfig.ModulePath,MAX_PATH,IniFilePath);

	CurrentConfig.KeyMap = GetPrivateProfileInt("Misc","KeyMapIndex",0,IniFilePath);
	if (CurrentConfig.KeyMap>3)
		CurrentConfig.KeyMap=0;	//Default to DECB Mapping

	GetPrivateProfileString("Misc","CustomKeyMapFile","",KeyMapFilePath,MAX_PATH,IniFilePath);
	if (*KeyMapFilePath == '\0') {
		strcpy(KeyMapFilePath, AppDataPath);
		strcat(KeyMapFilePath, "\\custom.keymap");
	}
	if (CurrentConfig.KeyMap == kKBLayoutCustom) LoadCustomKeyMap(KeyMapFilePath);
	vccKeyboardBuildRuntimeTable((keyboardlayout_e)CurrentConfig.KeyMap);

	CheckPath(CurrentConfig.ModulePath);

	LeftJS.UseMouse=GetPrivateProfileInt("LeftJoyStick","UseMouse",1,IniFilePath);
	LeftJS.Left=GetPrivateProfileInt("LeftJoyStick","Left",75,IniFilePath);
	LeftJS.Right=GetPrivateProfileInt("LeftJoyStick","Right",77,IniFilePath);
	LeftJS.Up=GetPrivateProfileInt("LeftJoyStick","Up",72,IniFilePath);
	LeftJS.Down=GetPrivateProfileInt("LeftJoyStick","Down",80,IniFilePath);
	LeftJS.Fire1=GetPrivateProfileInt("LeftJoyStick","Fire1",59,IniFilePath);
	LeftJS.Fire2=GetPrivateProfileInt("LeftJoyStick","Fire2",60,IniFilePath);
	LeftJS.DiDevice=GetPrivateProfileInt("LeftJoyStick","DiDevice",0,IniFilePath);
	LeftJS.HiRes= GetPrivateProfileInt("LeftJoyStick", "HiResDevice", 0, IniFilePath);
	RightJS.UseMouse=GetPrivateProfileInt("RightJoyStick","UseMouse",1,IniFilePath);
	RightJS.Left=GetPrivateProfileInt("RightJoyStick","Left",75,IniFilePath);
	RightJS.Right=GetPrivateProfileInt("RightJoyStick","Right",77,IniFilePath);
	RightJS.Up=GetPrivateProfileInt("RightJoyStick","Up",72,IniFilePath);
	RightJS.Down=GetPrivateProfileInt("RightJoyStick","Down",80,IniFilePath);
	RightJS.Fire1=GetPrivateProfileInt("RightJoyStick","Fire1",59,IniFilePath);
	RightJS.Fire2=GetPrivateProfileInt("RightJoyStick","Fire2",60,IniFilePath);
	RightJS.DiDevice=GetPrivateProfileInt("RightJoyStick","DiDevice",0,IniFilePath);
	RightJS.HiRes = GetPrivateProfileInt("RightJoyStick", "HiResDevice", 0, IniFilePath);

	GetPrivateProfileString("DefaultPaths", "CassPath", "", CurrentConfig.CassPath, MAX_PATH, IniFilePath);
	GetPrivateProfileString("DefaultPaths", "FloppyPath", "", CurrentConfig.FloppyPath, MAX_PATH, IniFilePath);

	for (Index = 0; Index < NumberOfSoundCards; Index++)
	{
		if (!strcmp(SoundCards[Index].CardName, CurrentConfig.SoundCardName))
		{
			CurrentConfig.SndOutDev = Index;
		}
	}

	CurrentConfig.Resize = 1; //Checkbox removed. Remove this from the ini?

	Rect rect = { CW_USEDEFAULT, CW_USEDEFAULT, DefaultWidth, DefaultHeight };
	if (CurrentConfig.RememberSize)
		rect = CurrentConfig.WindowRect;
	return(0);
}

void LoadModule()
{
	InsertModule(CurrentConfig.ModulePath);
}

void SetWindowRect(const Rect& rect) 
{
	if (EmuState.WindowHandle != NULL)
	{
		RECT ra = { 0,0,0,0 };
		::AdjustWindowRect(&ra, WS_OVERLAPPEDWINDOW, TRUE);
		int windowBorderWidth = ra.right - ra.left;
		int windowBorderHeight = ra.bottom - ra.top;
		::GetWindowRect(EmuState.WindowHandle, &ra);

		int width = rect.w + windowBorderWidth;
		int height = rect.h + windowBorderHeight + GetRenderWindowStatusBarHeight();
		int flags = SWP_NOOWNERZORDER | SWP_NOZORDER;
		int x = rect.IsDefaultX() ? ra.left : rect.x;
		int y = rect.IsDefaultY() ? ra.top : rect.y;
		SetWindowPos(EmuState.WindowHandle, 0, x, y, width, height, flags);
	}
}

/********************************************/
/*          Config File paths               */
/********************************************/
void GetIniFilePath( char *Path)
{
	strcpy(Path,IniFilePath);
	return;
}
void SetIniFilePath( char *Path)
{
    //  Path must be to an existing ini file
    strcpy(IniFilePath,Path);
}
// The following two functions only work after LoadConfig has been called
char * AppDirectory()
{
	return AppDataPath;
}
char * GetKeyMapFilePath()
{
	return KeyMapFilePath;
}
void SetKeyMapFilePath(char *Path)
{
    strcpy(KeyMapFilePath,Path);
	WritePrivateProfileString("Misc","CustomKeyMapFile",KeyMapFilePath,IniFilePath);
}

/*****************************************************/
/*  Apply Current VCC settings. Also Called by Vcc.c */
/*****************************************************/
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
	SetOverclock(CurrentConfig.EnableOverclock);

	if (CurrentConfig.BreakOpcEnabled) {
		EmuState.Debugger.Enable_Break(true);
	} else {
		EmuState.Debugger.Enable_Break(false);
	}

	SetMonitorType(CurrentConfig.MonitorType);
	SetCartAutoStart(CurrentConfig.CartAutoStart);
	if (CurrentConfig.RebootNow)
		DoReboot();
	CurrentConfig.RebootNow=0;
	EmuState.MousePointer = CurrentConfig.ShowMousePointer;
}

/********************************************/
/*               Cpu Config                 */
/********************************************/
HWND hCpuDlg = NULL;

void OpenCpuConfig() {
	if (hCpuDlg==NULL) {
		hCpuDlg = CreateDialog
			(EmuState.WindowInstance,(LPCTSTR) IDD_CPU,EmuState.WindowHandle,(DLGPROC) CpuConfig);
	}
	ShowWindow(hCpuDlg,SW_SHOWNORMAL);
	SetFocus(hCpuDlg);
}

LRESULT CALLBACK CpuConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	short int Ramchoice[4] = {IDC_128K,IDC_512K,IDC_2M,IDC_8M};
	short int Cpuchoice[2] = {IDC_6809,IDC_6309};
	unsigned char temp;
	static STRConfig tmpcfg;
	OPENFILENAME ofn;
	HWND hClkSpd = GetDlgItem(hDlg,IDC_CLOCKSPEED);
	HWND hClkDsp = GetDlgItem(hDlg,IDC_CLOCKDISPLAY);
	switch (message) {
	case WM_INITDIALOG:
		tmpcfg = CurrentConfig;
		CpuIcons[0]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_MOTO);
		CpuIcons[1]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_HITACHI2);
		SendMessage(hClkSpd,TBM_SETRANGE,TRUE,MAKELONG(2,CurrentConfig.MaxOverclock));
		sprintf(OutBuffer,"%2.3f Mhz",(float)CurrentConfig.CPUMultiplyer*.894);
		SendMessage(hClkDsp,WM_SETTEXT,0,(LPARAM)(LPCSTR)OutBuffer);
		SendMessage(hClkSpd,TBM_SETPOS,TRUE, CurrentConfig.CPUMultiplyer);
		for (temp=0;temp<=3;temp++)
			SendDlgItemMessage(hDlg,Ramchoice[temp],BM_SETCHECK,
					(temp==CurrentConfig.RamSize),0);
		for (temp=0;temp<=1;temp++)
			SendDlgItemMessage(hDlg,Cpuchoice[temp],BM_SETCHECK,
					(temp==CurrentConfig.CpuType),0);
		SendDlgItemMessage (hDlg,IDC_CPUICON,STM_SETIMAGE,(WPARAM)IMAGE_ICON,
			 (LPARAM)CpuIcons[CurrentConfig.CpuType]);
		SendDlgItemMessage(hDlg,IDC_ENABLE_BREAK,BM_SETCHECK,CurrentConfig.BreakOpcEnabled,0);
		SendDlgItemMessage(hDlg,IDC_AUTOSTART,BM_SETCHECK,tmpcfg.AutoStart,0);
		SendDlgItemMessage(hDlg,IDC_AUTOCART,BM_SETCHECK,tmpcfg.CartAutoStart,0);
		SendDlgItemMessage(hDlg,IDC_USE_EXTROM,BM_SETCHECK,tmpcfg.UseExtCocoRom,0);
		SendDlgItemMessage(hDlg,IDC_OVERCLOCK,BM_SETCHECK,tmpcfg.EnableOverclock,0);
		SetDlgItemText(hDlg,IDC_ROMPATH,tmpcfg.ExtRomFile);
		break;

	case WM_HSCROLL:
		tmpcfg.CPUMultiplyer = (unsigned char) SendMessage (hClkSpd,TBM_GETPOS,0,0);
		sprintf(OutBuffer,"%2.3f Mhz",(float)tmpcfg.CPUMultiplyer*.894);
		SendDlgItemMessage(hDlg,IDC_CLOCKDISPLAY,WM_SETTEXT,0,(LPARAM)(LPCSTR)OutBuffer);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
		case IDCLOSE:
			hCpuDlg = NULL;
			DestroyWindow(hDlg);
			break;
		case IDOK:
		case IDAPPLY:

			// Get coco3.rom path from dialog if external
			if (tmpcfg.UseExtCocoRom) {
				GetDlgItemText(hDlg,IDC_ROMPATH,tmpcfg.ExtRomFile,MAX_PATH);
			}

			// ResetPending causes Vcc.c to call UpdateConfig().
			if ( (CurrentConfig.RamSize != tmpcfg.RamSize) |
			     (CurrentConfig.CpuType != tmpcfg.CpuType) |
			     (CurrentConfig.UseExtCocoRom != tmpcfg.UseExtCocoRom) |
				 (strcmp(CurrentConfig.ExtRomFile,tmpcfg.ExtRomFile) != 0)) {
				EmuState.ResetPending = 2;   // Hard reset
			} else {
				EmuState.ResetPending = 4;   // Update config
			}
			// Commit any CPU modifications
			CurrentConfig.RamSize         = tmpcfg.RamSize;
			CurrentConfig.CpuType         = tmpcfg.CpuType;
			CurrentConfig.BreakOpcEnabled = tmpcfg.BreakOpcEnabled;
			CurrentConfig.CPUMultiplyer   = tmpcfg.CPUMultiplyer;
			// Set CPU icon in dialog window
			SendDlgItemMessage ( hDlg,IDC_CPUICON,STM_SETIMAGE,(WPARAM)IMAGE_ICON,
			                     (LPARAM)CpuIcons[CurrentConfig.CpuType] );
			CurrentConfig.AutoStart = tmpcfg.AutoStart;
			CurrentConfig.CartAutoStart = tmpcfg.CartAutoStart;
			CurrentConfig.UseExtCocoRom = tmpcfg.UseExtCocoRom;
			CurrentConfig.EnableOverclock = tmpcfg.EnableOverclock;
			strncpy(CurrentConfig.ExtRomFile,tmpcfg.ExtRomFile,MAX_PATH);
			SetOverclock(CurrentConfig.EnableOverclock);
			// Exit dialog if IDOK
			if (LOWORD(wParam)==IDOK) {
				hCpuDlg = NULL;
				DestroyWindow(hDlg);
			}
			break;
		case IDC_128K:
		case IDC_512K:
		case IDC_2M:
		case IDC_8M:
			for (temp=0;temp<=3;temp++) {
				if (LOWORD(wParam) == Ramchoice[temp]) {
					SendDlgItemMessage(hDlg,Ramchoice[temp],BM_SETCHECK,1,0);
					tmpcfg.RamSize=temp;
				} else {
					SendDlgItemMessage(hDlg,Ramchoice[temp],BM_SETCHECK,0,0);
				}
			}
			break;
		case IDC_6809:
		case IDC_6309:
			for (temp=0;temp<=1;temp++) {
				if (LOWORD(wParam) == Cpuchoice[temp]) {
					tmpcfg.CpuType = temp;
					SendDlgItemMessage(hDlg,Cpuchoice[temp],BM_SETCHECK,1,0);
				} else {
					SendDlgItemMessage(hDlg,Cpuchoice[temp],BM_SETCHECK,0,0);
				}
			}
			break;
		case IDC_ENABLE_BREAK:
			tmpcfg.BreakOpcEnabled = (unsigned char)
						SendDlgItemMessage(hDlg,IDC_ENABLE_BREAK,BM_GETCHECK,0,0);
			break;
		case IDC_AUTOSTART:
			tmpcfg.AutoStart = (unsigned char)
						SendDlgItemMessage(hDlg,IDC_AUTOSTART,BM_GETCHECK,0,0);
			break;
		case IDC_AUTOCART:
			tmpcfg.CartAutoStart = (unsigned char)
						SendDlgItemMessage(hDlg,IDC_AUTOCART,BM_GETCHECK,0,0);
			break;
		case IDC_USE_EXTROM:
			tmpcfg.UseExtCocoRom = (unsigned char)
						SendDlgItemMessage(hDlg,IDC_USE_EXTROM,BM_GETCHECK,0,0);
			break;
		case IDC_OVERCLOCK:
			tmpcfg.EnableOverclock = (unsigned char)
						SendDlgItemMessage(hDlg,IDC_OVERCLOCK,BM_GETCHECK,0,0);
			break;
		case IDC_BROWSE:
			memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize       = sizeof (OPENFILENAME) ;
			ofn.hwndOwner		  = NULL;
			ofn.lpstrFilter		  = "Rom Image\0*.rom\0\0";
			ofn.nFilterIndex      = 1;
			ofn.lpstrFile         = tmpcfg.ExtRomFile;
			ofn.nMaxFile          = MAX_PATH;
			ofn.lpstrFileTitle    = NULL;
			ofn.nMaxFileTitle     = MAX_PATH;
			ofn.lpstrInitialDir   = NULL;
			ofn.lpstrTitle        = TEXT("Coco3 Rom Image");
			ofn.Flags             = OFN_HIDEREADONLY;
			GetOpenFileName (&ofn);
			SetDlgItemText(hDlg,IDC_ROMPATH,tmpcfg.ExtRomFile);
			break;

		}		//End switch LOWORD(wParam)
		break;	//Break WM_COMMAND
	}			//END switch (message)
	return(0);
}

/* Set overclocking */
void SetOverclock(unsigned char flag)
{
	EmuState.OverclockFlag = flag;
    if (hCpuDlg != NULL)
		SendDlgItemMessage(hCpuDlg,IDC_OVERCLOCK,BM_SETCHECK,flag,0);
}

/* Increase the overclock speed (2..100), as seen after a POKE 65497,0. */
void IncreaseOverclockSpeed()
{
	if (CurrentConfig.CPUMultiplyer >= CurrentConfig.MaxOverclock) return;

	CurrentConfig.CPUMultiplyer = (unsigned char)(CurrentConfig.CPUMultiplyer + 1);

	// Send updates to the dialog if it's open.
    if (hCpuDlg != NULL) {
		SendDlgItemMessage(hCpuDlg, IDC_CLOCKSPEED, TBM_SETPOS,
						   TRUE, CurrentConfig.CPUMultiplyer);
		sprintf(OutBuffer, "%2.3f Mhz", (float)CurrentConfig.CPUMultiplyer * 0.894);
		SendDlgItemMessage(hCpuDlg, IDC_CLOCKDISPLAY, WM_SETTEXT,
						   0, (LPARAM)(LPCSTR)OutBuffer);
	}
	EmuState.ResetPending = 4; // Without this, changing the config does nothing.
}

/* Decrease the overclock speed */
void DecreaseOverclockSpeed()
{
	if (CurrentConfig.CPUMultiplyer == 2) return;

	CurrentConfig.CPUMultiplyer = (unsigned char)(CurrentConfig.CPUMultiplyer - 1);

	// Send updates to the dialog if it's open.
    if (hCpuDlg != NULL) {
		SendDlgItemMessage(hCpuDlg, IDC_CLOCKSPEED, TBM_SETPOS,
						   TRUE, CurrentConfig.CPUMultiplyer);
		sprintf(OutBuffer, "%2.3f Mhz", (float)CurrentConfig.CPUMultiplyer * 0.894);
		SendDlgItemMessage(hCpuDlg, IDC_CLOCKDISPLAY, WM_SETTEXT,
						   0, (LPARAM)(LPCSTR)OutBuffer);
	}
	EmuState.ResetPending = 4;
}

/********************************************/
/*               Tape Config                */
/********************************************/
HWND hTapeDlg = NULL;
void OpenTapeConfig() {
	if (hTapeDlg==NULL) {
		hTapeDlg = CreateDialog
			( EmuState.WindowInstance, (LPCTSTR) IDD_CASSETTE,
			  EmuState.WindowHandle,   (DLGPROC) TapeConfig );
	}
	ShowWindow(hTapeDlg,SW_SHOWNORMAL);
	SetFocus(hTapeDlg);
}
LRESULT CALLBACK TapeConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	CounterText.cbSize = sizeof(CHARFORMAT);
	CounterText.dwMask = CFM_BOLD | CFM_COLOR ;
	CounterText.dwEffects = CFE_BOLD;
	CounterText.crTextColor=RGB(255,255,255);

	ModeText.cbSize = sizeof(CHARFORMAT);
	ModeText.dwMask = CFM_BOLD | CFM_COLOR ;
	ModeText.dwEffects = CFE_BOLD;
	ModeText.crTextColor=RGB(255,0,0);

	switch (message) {
	case WM_INITDIALOG:
		TapeCounter=GetTapeCounter();
		sprintf(OutBuffer,"%i",TapeCounter);
		SendDlgItemMessage(hDlg,IDC_TCOUNT,WM_SETTEXT,0,(LPARAM)(LPCSTR)OutBuffer);
		SendDlgItemMessage(hDlg,IDC_MODE,WM_SETTEXT,0,(LPARAM)(LPCSTR)Tmodes[Tmode]);
		GetTapeName(TapeFileName);  // Defined in Cassette.cpp
		SendDlgItemMessage(hDlg,IDC_TAPEFILE,WM_SETTEXT,0,(LPARAM)(LPCSTR)TapeFileName);
		SendDlgItemMessage(hDlg,IDC_TCOUNT,EM_SETBKGNDCOLOR ,0,(LPARAM)RGB(0,0,0));
		SendDlgItemMessage(hDlg,IDC_TCOUNT,EM_SETCHARFORMAT ,SCF_ALL,(LPARAM)&CounterText);
		SendDlgItemMessage(hDlg,IDC_MODE,EM_SETBKGNDCOLOR ,0,(LPARAM)RGB(0,0,0));
		SendDlgItemMessage(hDlg,IDC_MODE,EM_SETCHARFORMAT ,SCF_ALL,(LPARAM)&CounterText);
		SendDlgItemMessage(hDlg,IDC_FASTLOAD, BM_SETCHECK, TapeFastLoad, 0);
		break;

	case WM_COMMAND:
		switch (LOWORD (wParam)) {
		case IDCANCEL:
		case IDCLOSE:
			hTapeDlg = NULL;
			DestroyWindow(hDlg);
			break;
		case IDOK:
		case IDAPPLY:
			UpdateConfig();
			if (LOWORD(wParam)==IDOK) {
				hTapeDlg = NULL;
				DestroyWindow(hDlg);
			}
			break;
		case IDC_PLAY:
			SetTapeMode(PLAY);
			break;
		case IDC_REC:
			SetTapeMode(REC);
			break;
		case IDC_STOP:
			SetTapeMode(STOP);
			break;
		case IDC_EJECT:
			SetTapeMode(EJECT);
			break;
		case IDC_RESET:
			SetTapeCounter(0);
			SetTapeMode(STOP);
			break;
		case IDC_TBROWSE:
			LoadTape();
			SendDlgItemMessage(hDlg, IDC_FASTLOAD, BM_SETCHECK, TapeFastLoad, 0);
			SetTapeCounter(0, true);
			break;
		case IDC_FASTLOAD:
			TapeFastLoad = (unsigned char)SendDlgItemMessage(hDlg, IDC_FASTLOAD, BM_GETCHECK, 0, 0);
			break;
		}
		break;	//End WM_COMMAND
	}
	return(0);
}

void UpdateTapeCounter(unsigned int Counter,unsigned char TapeMode, bool forced)
{
	if (hTapeDlg==NULL) return;

	if (Counter != TapeCounter || forced)
	{
		TapeCounter = Counter;
		sprintf(OutBuffer, "%i", TapeCounter);
		SendDlgItemMessage(hTapeDlg, IDC_TCOUNT,
			WM_SETTEXT, 0, (LPARAM)(LPCSTR)OutBuffer);
	}

	if (Tmode != TapeMode || forced)
	{
		Tmode = TapeMode;
		SendDlgItemMessage(hTapeDlg, IDC_MODE,
			WM_SETTEXT, 0, (LPARAM)(LPCSTR)Tmodes[Tmode]);
		GetTapeName(TapeFileName);
		PathStripPath(TapeFileName);
		SendDlgItemMessage(hTapeDlg, IDC_TAPEFILE,
			WM_SETTEXT, 0, (LPARAM)(LPCSTR)TapeFileName);

		switch (Tmode) {
			case REC:
				SendDlgItemMessage(hTapeDlg, IDC_MODE, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0xAF, 0, 0));
				break;

			case PLAY:
				SendDlgItemMessage(hTapeDlg, IDC_MODE, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0xAF, 0));
				break;

			default:
				SendDlgItemMessage(hTapeDlg, IDC_MODE, EM_SETBKGNDCOLOR, 0, (LPARAM)RGB(0, 0, 0));
				break;
		}
	}
}

/********************************************/
/*              Audio Config                */
/********************************************/
HWND hAudioDlg = NULL;
void OpenAudioConfig() {
	if (hAudioDlg==NULL) {
		hAudioDlg = CreateDialog
			(EmuState.WindowInstance,(LPCTSTR) IDD_AUDIO,EmuState.WindowHandle,(DLGPROC) AudioConfig);
	}
	ShowWindow(hAudioDlg,SW_SHOWNORMAL);
	SetFocus(hAudioDlg);
}

LRESULT CALLBACK AudioConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Index;
	static STRConfig tmpcfg;
	WPARAM bstate;
	switch (message) {
	case WM_INITDIALOG:
		tmpcfg = CurrentConfig;

		// Sound level bars (L and R are equal)
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
			SendDlgItemMessage(hDlg,IDC_SOUNDCARD,CB_ADDSTRING,
			                  (WPARAM)0,(LPARAM)SoundCards[Index].CardName);

//		In coco3.cpp audio rate is forced to 0 (mute) or 3 (44100)
//		Rate dropdown has been replaced with a mute check box
		bstate = (tmpcfg.AudioRate==0) ? BST_CHECKED : BST_UNCHECKED;
		SendDlgItemMessage(hDlg,IDC_MUTE,BM_SETCHECK,bstate,0);
//		for (Index=0;Index<4;Index++)
//			SendDlgItemMessage(hDlg,IDC_RATE,CB_ADDSTRING,
//			                   (WPARAM)0,(LPARAM)RateList[Index]);
//		SendDlgItemMessage(hDlg,IDC_RATE,CB_SETCURSEL,
//		                   (WPARAM)tmpcfg.AudioRate,(LPARAM)0);

		// Sound device selection
		tmpcfg.SndOutDev=0;
		for (Index=0;Index<NumberOfSoundCards;Index++)
			if (!strcmp(SoundCards[Index].CardName,tmpcfg.SoundCardName))
				tmpcfg.SndOutDev=Index;
		SendDlgItemMessage(hDlg,IDC_SOUNDCARD,CB_SETCURSEL,
		                   (WPARAM)tmpcfg.SndOutDev,(LPARAM)0);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
		case IDCLOSE:
			hAudioDlg = NULL;
			DestroyWindow(hDlg);
			break;
		case IDOK:
		case IDAPPLY:
			tmpcfg.SndOutDev =
				(unsigned char) SendDlgItemMessage(hDlg,IDC_SOUNDCARD,CB_GETCURSEL,0,0);
			tmpcfg.AudioRate =
				(SendDlgItemMessage(hDlg,IDC_MUTE,BM_GETCHECK,0,0) == BST_CHECKED)? 0:3;

			if ( (CurrentConfig.SndOutDev != tmpcfg.SndOutDev) |
			     (CurrentConfig.AudioRate != tmpcfg.AudioRate)) {
				SoundInit ( EmuState.WindowHandle,
				            SoundCards[tmpcfg.SndOutDev].Guid,
				            tmpcfg.AudioRate);
			}
			CurrentConfig.AudioRate = tmpcfg.AudioRate;
			CurrentConfig.SndOutDev = tmpcfg.SndOutDev;
			strcpy(CurrentConfig.SoundCardName, SoundCards[CurrentConfig.SndOutDev].CardName);
			UpdateConfig();
			if (LOWORD(wParam)==IDOK) {
				hAudioDlg = NULL;
				DestroyWindow(hDlg);
			}
			break;
		}
		break;
	}
	return(0);
}

void UpdateSoundBar(unsigned int * audioBuf,unsigned int bufLen)
{
	if (hAudioDlg == NULL) return;  // Do nothing if dialog is not running

	// Craig's method moved from audio.c with liberties taken for variable naming
	// Get mid level for 100 samples, left and right
	int lval = audioBuf[0] >> 16;
	int rval = audioBuf[0] & 0xFFFF;
	int lMin = lval;
	int lMax = lval;
	int rMin = rval;
	int rMax = rval;
	for (size_t i = 1; i < min(bufLen,100); ++i) {
		int lval = audioBuf[i] >> 16;
		int rval = audioBuf[i] & 0xFFFF;
		if (lval > lMax) lMax = lval;
		if (lval < lMin) lMin = lval;
		if (rval > rMax) rMax = rval;
		if (rval < rMin) rMin = rval;
	}
	int midLeft = lMax - lMin;
	int midRight = rMax - rMin;

	// Smoothing
	static int lastLeft = 0;
	static int lastRight = 0;
	if (midLeft > lastLeft) lastLeft = midLeft;
	if (midRight > lastRight) lastRight = midRight;
	int Left = (lastLeft * 3 + midLeft) >> 2;
	int Right = (lastRight * 3 + midRight) >> 2;
	lastLeft = Left;
	lastRight = Right;

	SendDlgItemMessage(hAudioDlg,IDC_PROGRESSLEFT,PBM_SETPOS,Left/180,0);
	SendDlgItemMessage(hAudioDlg,IDC_PROGRESSRIGHT,PBM_SETPOS,Right/180,0);

	return;
}

/********************************************/
/*              Display Config              */
/********************************************/
HWND hDisplayDlg = NULL;
void OpenDisplayConfig() {
	if (hDisplayDlg == NULL) {
		hDisplayDlg = CreateDialog
			(EmuState.WindowInstance,(LPCTSTR) IDD_DISPLAY,
			 EmuState.WindowHandle,(DLGPROC) DisplayConfig);
	}
	ShowWindow(hDisplayDlg,SW_SHOWNORMAL);
	SetFocus(hDisplayDlg);
}

LRESULT CALLBACK DisplayConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static STRConfig tmpcfg;
	switch (message) {
	case WM_INITDIALOG:
		tmpcfg = CurrentConfig;
		MonIcons[0]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_COMPOSITE);
		MonIcons[1]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_RGB);

		SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_SETRANGE,TRUE,MAKELONG(1,6) );
		SendDlgItemMessage(hDlg,IDC_SCANLINES,BM_SETCHECK,tmpcfg.ScanLines,0);
		SendDlgItemMessage(hDlg,IDC_THROTTLE,BM_SETCHECK,tmpcfg.SpeedThrottle,0);
		SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_SETPOS,TRUE,tmpcfg.FrameSkip);
		SendDlgItemMessage(hDlg,IDC_RESIZE,BM_SETCHECK,tmpcfg.Resize,0);
		SendDlgItemMessage(hDlg,IDC_ASPECT,BM_SETCHECK,tmpcfg.Aspect,0);
		SendDlgItemMessage(hDlg,IDC_REMEMBER_SIZE, BM_SETCHECK, tmpcfg.RememberSize, 0);
		sprintf(OutBuffer,"%i",tmpcfg.FrameSkip);
		SendDlgItemMessage(hDlg,IDC_FRAMEDISPLAY,
				WM_SETTEXT,0,(LPARAM)(LPCSTR)OutBuffer);
		if (tmpcfg.MonitorType == 1) {
			SendDlgItemMessage(hDlg,IDC_COMPOSITE,BM_SETCHECK,0,0);
			SendDlgItemMessage(hDlg,IDC_RGB,BM_SETCHECK,1,0);
		} else {
			SendDlgItemMessage(hDlg,IDC_COMPOSITE,BM_SETCHECK,1,0);
			SendDlgItemMessage(hDlg,IDC_RGB,BM_SETCHECK,0,0);
		}
		SendDlgItemMessage(hDlg,IDC_MONTYPE,
				STM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)MonIcons[tmpcfg.MonitorType]);
		if (tmpcfg.PaletteType == PALETTE_NTSC) {
			SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETCHECK, 0, 0);
			SendDlgItemMessage(hDlg, IDC_NTSC5_PALETTE, BM_SETCHECK, 1, 0);
		} else {
			SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETCHECK, 1, 0);
			SendDlgItemMessage(hDlg, IDC_NTSC5_PALETTE, BM_SETCHECK, 0, 0);
		}
		break;

	case WM_HSCROLL:
		tmpcfg.FrameSkip=(unsigned char)
			SendDlgItemMessage(hDlg,IDC_FRAMESKIP,TBM_GETPOS,(WPARAM) 0, (WPARAM) 0);
		sprintf(OutBuffer,"%i",tmpcfg.FrameSkip);
		SendDlgItemMessage(hDlg,IDC_FRAMEDISPLAY,
			WM_SETTEXT,0,(LPARAM)(LPCSTR)OutBuffer);
		break;

	case WM_COMMAND:
		tmpcfg.Resize = 1;//(unsigned char)SendDlgItemMessage(hDlg,IDC_RESIZE,BM_GETCHECK,0,0);
		tmpcfg.Aspect = (unsigned char) SendDlgItemMessage(hDlg,IDC_ASPECT,BM_GETCHECK,0,0);
		tmpcfg.ScanLines = (unsigned char) SendDlgItemMessage(hDlg,IDC_SCANLINES,BM_GETCHECK,0,0);
		tmpcfg.SpeedThrottle = (unsigned char)
			SendDlgItemMessage(hDlg,IDC_THROTTLE,BM_GETCHECK,0,0);
		tmpcfg.RememberSize = (unsigned char)
			SendDlgItemMessage(hDlg,IDC_REMEMBER_SIZE,BM_GETCHECK,0,0);
		//POINT p = { 640,480 };
		switch (LOWORD (wParam)) {

		case IDCANCEL:
		case IDCLOSE:
			hDisplayDlg = NULL;
			DestroyWindow(hDlg);
			break;
		case IDOK:
		case IDAPPLY:
			CurrentConfig.FrameSkip     = tmpcfg.FrameSkip;
			CurrentConfig.Resize        = tmpcfg.Resize;
			CurrentConfig.Aspect        = tmpcfg.Aspect;
			CurrentConfig.ScanLines     = tmpcfg.ScanLines;
			CurrentConfig.SpeedThrottle = tmpcfg.SpeedThrottle;
			CurrentConfig.RememberSize  = tmpcfg.RememberSize;
			CurrentConfig.Resize        = tmpcfg.Resize;
			CurrentConfig.MonitorType   = tmpcfg.MonitorType;
			CurrentConfig.PaletteType   = tmpcfg.PaletteType;
			UpdateConfig();
			if (LOWORD(wParam)==IDOK) {
				hDisplayDlg = NULL;
				DestroyWindow(hDlg);
			}
			break;

		case IDC_REMEMBER_SIZE:
			tmpcfg.Resize = 1;
			SendDlgItemMessage(hDlg, IDC_RESIZE, BM_GETCHECK, 1, 0);
			break;

		case IDC_RGB:
			SendDlgItemMessage(hDlg, IDC_COMPOSITE, BM_SETCHECK, 0, 0);
			SendDlgItemMessage(hDlg, IDC_RGB, BM_SETCHECK, 1, 0);
			tmpcfg.MonitorType = 1;
			SendDlgItemMessage(hDlg,IDC_MONTYPE,
				STM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)MonIcons[1]);
			break;

		case IDC_COMPOSITE:
			SendDlgItemMessage(hDlg, IDC_COMPOSITE, BM_SETCHECK, 1, 0);
			SendDlgItemMessage(hDlg, IDC_RGB, BM_SETCHECK, 0, 0);
			tmpcfg.MonitorType = 0;
			SendDlgItemMessage(hDlg,IDC_MONTYPE,
				STM_SETIMAGE,(WPARAM)IMAGE_ICON,(LPARAM)MonIcons[0]);
			break;

		case IDC_UPD_PALETTE:
			SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETCHECK, 1, 0);
			SendDlgItemMessage(hDlg, IDC_NTSC5_PALETTE, BM_SETCHECK, 0, 0);
			tmpcfg.PaletteType = PALETTE_UPD;
			SendMessage(hDlg,WM_COMMAND,IDC_COMPOSITE,0);
			break;

		case IDC_NTSC5_PALETTE:
			SendDlgItemMessage(hDlg, IDC_UPD_PALETTE, BM_SETCHECK, 0, 0);
			SendDlgItemMessage(hDlg, IDC_NTSC5_PALETTE, BM_SETCHECK, 1, 0);
			tmpcfg.PaletteType = PALETTE_NTSC;
			SendMessage(hDlg,WM_COMMAND,IDC_COMPOSITE,0);
			break;
			
		}	//End switch LOWORD(wParam)
		break;	//break WM_COMMAND
	}			//END switch Message
	return(0);
}

/********************************************/
/*             Keyboard Config              */
/********************************************/

int SetCurrentKeyMap(int keymap);
int SelectKeymapFile(HWND hdlg);
int ShowKeymapStatus(HWND hDlg);

HWND hInputDlg = NULL;
void OpenInputConfig() {
	if (hInputDlg==NULL) {
		hInputDlg = CreateDialog
			( EmuState.WindowInstance, (LPCTSTR) IDD_INPUT,
			  EmuState.WindowHandle,   (DLGPROC) InputConfig );
	}
	ShowWindow(hInputDlg,SW_SHOWNORMAL);
	SetFocus(hInputDlg);
}
LRESULT CALLBACK InputConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        ShowKeymapStatus(hDlg);
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
		case IDCANCEL:
		case IDCLOSE:
            hInputDlg = NULL;
			DestroyWindow(hDlg);
			break;
		case IDOK:
		case IDAPPLY:
			vccKeyboardBuildRuntimeTable((keyboardlayout_e)CurrentConfig.KeyMap);
			UpdateConfig();
			if (LOWORD(wParam)==IDOK) {
				hInputDlg = NULL;
				DestroyWindow(hDlg);
			}
			break;
		case IDC_KEYMAP_COCO:
            SetCurrentKeyMap(kKBLayoutCoCo);
            break;
        case IDC_KEYMAP_NATURAL:
            SetCurrentKeyMap(kKBLayoutNatural);
            break;
        case IDC_KEYMAP_COMPACT:
            SetCurrentKeyMap(kKBLayoutCompact);
            break;
        case IDC_KEYMAP_CUSTOM:
            SetCurrentKeyMap(kKBLayoutCustom);
            break;
        case IDC_SELECT_KEYMAP:
            SelectKeymapFile(hDlg);
            break;
        case IDC_KEYMAPED:
            SetCurrentKeyMap(kKBLayoutCustom);
            DialogBox( EmuState.WindowInstance,
                       (LPCTSTR) IDD_KEYMAPEDIT,
                       hDlg, (DLGPROC) KeyMapProc );
            break;
        }
        ShowKeymapStatus(hDlg);
        break;
    }
    return(0);
}

int SetCurrentKeyMap(int keymap) {
    // Force any changes to take immediate effect
    if (keymap != CurrentConfig.KeyMap) {
        vccKeyboardBuildRuntimeTable((keyboardlayout_e)keymap);
    }
    CurrentConfig.KeyMap = keymap;
    return keymap;
}

int ShowKeymapStatus(HWND hDlg)
{
    int i;
    int Btn[] = { IDC_KEYMAP_COCO,
                  IDC_KEYMAP_NATURAL,
                  IDC_KEYMAP_COMPACT,
                  IDC_KEYMAP_CUSTOM };
    for (i=0;i<4;i++) {
        if ( CurrentConfig.KeyMap == i) {
            SendDlgItemMessage(hDlg,Btn[i],BM_SETCHECK,1,0);
        } else {
            SendDlgItemMessage(hDlg,Btn[i],BM_SETCHECK,0,0);
        }
    }
    SendDlgItemMessage(hDlg,IDC_KEYMAP_FILE,WM_SETTEXT,0,(LPARAM)KeyMapFilePath);
    return(0);
}

BOOL SelectKeymapFile(HWND hDlg)
{

    OPENFILENAME ofn ;
    char FileSelected[MAX_PATH];
    strncpy (FileSelected,KeyMapFilePath,MAX_PATH);
    DWORD attr;

    // Prompt user for filename
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize     = sizeof (OPENFILENAME);
    ofn.hwndOwner       = hDlg;
    ofn.lpstrFilter     = "Keymap Files\0*.keymap\0\0";
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = FileSelected;
    ofn.nMaxFile        = MAX_PATH;
    ofn.lpstrFileTitle  = NULL;
    ofn.nMaxFileTitle   = MAX_PATH;
    ofn.lpstrInitialDir = AppDirectory();
    ofn.lpstrTitle      = TEXT("Select Keymap file");
    ofn.Flags           = OFN_HIDEREADONLY
                        | OFN_PATHMUSTEXIST;

    if ( GetOpenFileName (&ofn)) {
    if (ofn.nFileExtension==0) strcat(FileSelected,".keymap");
        // Load keymap if file exists
        attr=GetFileAttributesA(FileSelected);
        if ( (attr != INVALID_FILE_ATTRIBUTES) &&
            !(attr & FILE_ATTRIBUTE_DIRECTORY) ) {
            LoadCustomKeyMap(FileSelected);
        // Else create new file from current selection
        } else {
            char txt[MAX_PATH+32];
            strcpy (txt,"Create ");
            strcat (txt,FileSelected);
            strcat (txt,"?");
            if (MessageBox(hDlg,txt,"Warning",MB_YESNO)==IDYES) {
                CloneStandardKeymap(CurrentConfig.KeyMap);
                SaveCustomKeyMap(FileSelected);
            }
        }
        strncpy (KeyMapFilePath,FileSelected,MAX_PATH);
        SetKeyMapFilePath(KeyMapFilePath); // Save filename in Vcc.config
    }
	return TRUE;
}

// Called by Keyboard.c and KeyboardEdit.c
int GetKeyboardLayout() {
	return(CurrentConfig.KeyMap);
}

/********************************************/
/*              JoyStick Config             */
/********************************************/
HWND hJoyStickDlg = NULL;

void OpenJoyStickConfig() {
	if (hJoyStickDlg==NULL) {
		hJoyStickDlg = CreateDialog
			( EmuState.WindowInstance, (LPCTSTR) IDD_JOYSTICK,
			  EmuState.WindowHandle,   (DLGPROC) JoyStickConfig );
	}
	ShowWindow(hJoyStickDlg,SW_SHOWNORMAL);
	SetFocus(hJoyStickDlg);
}

LRESULT CALLBACK JoyStickConfig(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static STRConfig tmpcfg;
    static JoyStick TempLeftJS, TempRightJS;
	static int LeftJoyStick[6] = { IDC_LEFT_LEFT,IDC_LEFT_RIGHT,IDC_LEFT_UP,
		                           IDC_LEFT_DOWN,IDC_LEFT_FIRE1,IDC_LEFT_FIRE2 };
	static int RightJoyStick[6] = { IDC_RIGHT_LEFT,IDC_RIGHT_RIGHT,IDC_RIGHT_UP,
		                            IDC_RIGHT_DOWN,IDC_RIGHT_FIRE1,IDC_RIGHT_FIRE2 };
	static int LeftRadios[4] = { IDC_LEFT_KEYBOARD,IDC_LEFT_USEMOUSE,
		                         IDC_LEFTAUDIO,IDC_LEFTJOYSTICK };
	static int RightRadios[4] = { IDC_RIGHT_KEYBOARD,IDC_RIGHT_USEMOUSE,
		                          IDC_RIGHTAUDIO,IDC_RIGHTJOYSTICK };
	static int LeftJoyEmu[4] = { IDC_LEFTSTANDARD,IDC_LEFTSHIRES,
		                         IDC_LEFTTHIRES,IDC_LEFTCCMAX };
	static int RightJoyEmu[4] = { IDC_RIGHTSTANDARD, IDC_RIGHTSHIRES,
		                          IDC_RIGHTTHRES,IDC_RIGHTCCMAX };

    WPARAM showmouse = (EmuState.MousePointer == 0) ? BST_UNCHECKED : BST_CHECKED;

	switch (message)
	{
		case WM_INITDIALOG:
			RefreshJoystickStatus();
			tmpcfg = CurrentConfig;
			JoystickIcons[0]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_KEYBOARD);
			JoystickIcons[1]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_MOUSE);
			JoystickIcons[2]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_AUDIO);
			JoystickIcons[3]=LoadIcon(EmuState.WindowInstance,(LPCTSTR)IDI_JOYSTICK);
			for (temp = 0; temp < 68; temp++) {
				for (temp2 = 0; temp2 < 6; temp2++) {
					SendDlgItemMessage(hDlg, LeftJoyStick[temp2],
						CB_ADDSTRING, (WPARAM)0, (LPARAM)KEYNAME(temp));
					SendDlgItemMessage(hDlg, RightJoyStick[temp2],
						CB_ADDSTRING, (WPARAM)0, (LPARAM)KEYNAME(temp));
				}
			}

			for (temp = 0; temp < 6; temp++) {
				EnableWindow(GetDlgItem(hDlg, LeftJoyStick[temp]), (LeftJS.UseMouse == 0));
			}
			for (temp = 0; temp < 6; temp++) {
				EnableWindow(GetDlgItem(hDlg, RightJoyStick[temp]), (RightJS.UseMouse == 0));
			}

			for (temp=0;temp<=3;temp++) {
				SendDlgItemMessage(hDlg, LeftJoyEmu[temp], BM_SETCHECK, (temp == LeftJS.HiRes), 0);
				SendDlgItemMessage(hDlg, RightJoyEmu[temp], BM_SETCHECK, (temp == RightJS.HiRes), 0);
			}

			EnableWindow( GetDlgItem(hDlg,IDC_LEFTAUDIODEVICE),(LeftJS.UseMouse==2));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTAUDIODEVICE),(RightJS.UseMouse==2));
			EnableWindow( GetDlgItem(hDlg,IDC_LEFTJOYSTICKDEVICE),(LeftJS.UseMouse==3));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTJOYSTICKDEVICE),(RightJS.UseMouse==3));

			//Grey the Joystick Radios if No Joysticks are present
			EnableWindow( GetDlgItem(hDlg,IDC_LEFTJOYSTICK),(NumberofJoysticks>0));
			EnableWindow( GetDlgItem(hDlg,IDC_RIGHTJOYSTICK),(NumberofJoysticks>0));

			SendDlgItemMessage(hDlg,IDC_SHOWMOUSE,BM_SETCHECK,showmouse,(LPARAM(0)));

            //populate joystick combo boxes
			for(temp=0;temp<NumberofJoysticks;temp++) {
				SendDlgItemMessage(hDlg,IDC_RIGHTJOYSTICKDEVICE,
						CB_ADDSTRING,(WPARAM)0,(LPARAM)StickName[temp]);
				SendDlgItemMessage(hDlg,IDC_LEFTJOYSTICKDEVICE,
						CB_ADDSTRING,(WPARAM)0,(LPARAM)StickName[temp]);
			}
			SendDlgItemMessage(hDlg,IDC_RIGHTJOYSTICKDEVICE,
					CB_SETCURSEL,(WPARAM)RightJS.DiDevice,(LPARAM)0);
			SendDlgItemMessage(hDlg,IDC_LEFTJOYSTICKDEVICE,
					CB_SETCURSEL,(WPARAM)LeftJS.DiDevice,(LPARAM)0);

			SendDlgItemMessage(hDlg,LeftJoyStick[0],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(LeftJS.Left),(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[1],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(LeftJS.Right),(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[2],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(LeftJS.Up),(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[3],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(LeftJS.Down),(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[4],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(LeftJS.Fire1),(LPARAM)0);
			SendDlgItemMessage(hDlg,LeftJoyStick[5],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(LeftJS.Fire2),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[0],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(RightJS.Left),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[1],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(RightJS.Right),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[2],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(RightJS.Up),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[3],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(RightJS.Down),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[4],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(RightJS.Fire1),(LPARAM)0);
			SendDlgItemMessage(hDlg,RightJoyStick[5],
					CB_SETCURSEL,(WPARAM)TranslateScan2Disp(RightJS.Fire2),(LPARAM)0);

			for (temp = 0; temp <= 3; temp++) {
				SendDlgItemMessage(hDlg, LeftRadios[temp], BM_SETCHECK, temp == LeftJS.UseMouse, 0);
			}
			for (temp = 0; temp <= 3; temp++) {
				SendDlgItemMessage(hDlg, RightRadios[temp], BM_SETCHECK, temp == RightJS.UseMouse, 0);
			}
			TempLeftJS = LeftJS;
			TempRightJS = RightJS;
			SendDlgItemMessage(hDlg,IDC_LEFTICON,
					STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[LeftJS.UseMouse]);
			SendDlgItemMessage(hDlg,IDC_RIGHTICON,
					STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[RightJS.UseMouse]);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDCANCEL:
		case IDCLOSE:
			hJoyStickDlg = NULL;
			DestroyWindow(hDlg);
			break;
		case IDOK:
		case IDAPPLY:
			RightJS = TempRightJS;
			LeftJS = TempLeftJS;
			SetStickNumbers(LeftJS.DiDevice,RightJS.DiDevice);
			CurrentConfig.ShowMousePointer = tmpcfg.ShowMousePointer;
			UpdateConfig();
			if (LOWORD(wParam)==IDOK) {
				hJoyStickDlg = NULL;
				DestroyWindow(hDlg);
			}
			break;
		case IDC_SHOWMOUSE:
			if (HIWORD(wParam) == BN_CLICKED) {
				EmuState.MousePointer = (unsigned char) SendDlgItemMessage(hDlg, IDC_SHOWMOUSE,
						BM_GETCHECK, 0, 0);
				tmpcfg.ShowMousePointer = EmuState.MousePointer;
			}
			break;
		} // End switch WM_COMMAND wParam

		for (temp = 0; temp <= 3; temp++) {
			if (LOWORD(wParam) == LeftRadios[temp]) {
				for (temp2 = 0; temp2 <= 3; temp2++)
					SendDlgItemMessage(hDlg, LeftRadios[temp2], BM_SETCHECK, 0, 0);
				SendDlgItemMessage(hDlg, LeftRadios[temp], BM_SETCHECK, 1, 0);
				TempLeftJS.UseMouse = temp;
			}
		}

		for (temp = 0; temp <= 3; temp++) {
			if (LOWORD(wParam) == RightRadios[temp]) {
				for (temp2 = 0; temp2 <= 3; temp2++)
					SendDlgItemMessage(hDlg, RightRadios[temp2], BM_SETCHECK, 0, 0);
				SendDlgItemMessage(hDlg, RightRadios[temp], BM_SETCHECK, 1, 0);
				TempRightJS.UseMouse = temp;
			}
		}

		for (temp = 0; temp <= 3; temp++) {
			if (LOWORD(wParam) == LeftJoyEmu[temp]) {
				for (temp2 = 0; temp2 <= 3; temp2++)
					SendDlgItemMessage(hDlg, LeftJoyEmu[temp2], BM_SETCHECK, 0, 0);
				SendDlgItemMessage(hDlg, LeftJoyEmu[temp], BM_SETCHECK, 1, 0);
				TempLeftJS.HiRes = temp;
			}
		}

		for (temp = 0; temp <= 3; temp++) {
			if (LOWORD(wParam) == RightJoyEmu[temp]) {
				for (temp2 = 0; temp2 <= 3; temp2++)
					SendDlgItemMessage(hDlg, RightJoyEmu[temp2], BM_SETCHECK, 0, 0);
				SendDlgItemMessage(hDlg, RightJoyEmu[temp], BM_SETCHECK, 1, 0);
				TempRightJS.HiRes = temp;
			}
		}

		for (temp = 0; temp < 6; temp++) {
			EnableWindow(GetDlgItem(hDlg, LeftJoyStick[temp]), (TempLeftJS.UseMouse == 0));
		}

		for (temp = 0; temp < 6; temp++) {
			EnableWindow(GetDlgItem(hDlg, RightJoyStick[temp]), (TempRightJS.UseMouse == 0));
		}

		EnableWindow( GetDlgItem(hDlg,IDC_LEFTAUDIODEVICE),(TempLeftJS.UseMouse==2));
		EnableWindow( GetDlgItem(hDlg,IDC_RIGHTAUDIODEVICE),(TempRightJS.UseMouse==2));
		EnableWindow( GetDlgItem(hDlg,IDC_LEFTJOYSTICKDEVICE),(TempLeftJS.UseMouse==3));
		EnableWindow( GetDlgItem(hDlg,IDC_RIGHTJOYSTICKDEVICE),(TempRightJS.UseMouse==3));

		TempLeftJS.Left=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,LeftJoyStick[0],CB_GETCURSEL,0,0));
		TempLeftJS.Right=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,LeftJoyStick[1],CB_GETCURSEL,0,0));
		TempLeftJS.Up=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,LeftJoyStick[2],CB_GETCURSEL,0,0));
		TempLeftJS.Down=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,LeftJoyStick[3],CB_GETCURSEL,0,0));
		TempLeftJS.Fire1=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,LeftJoyStick[4],CB_GETCURSEL,0,0));
		TempLeftJS.Fire2=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,LeftJoyStick[5],CB_GETCURSEL,0,0));

		TempRightJS.Left=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,RightJoyStick[0],CB_GETCURSEL,0,0));
		TempRightJS.Right=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,RightJoyStick[1],CB_GETCURSEL,0,0));
		TempRightJS.Up=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,RightJoyStick[2],CB_GETCURSEL,0,0));
		TempRightJS.Down=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,RightJoyStick[3],CB_GETCURSEL,0,0));
		TempRightJS.Fire1=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,RightJoyStick[4],CB_GETCURSEL,0,0));
		TempRightJS.Fire2=TranslateDisp2Scan(
				SendDlgItemMessage(hDlg,RightJoyStick[5],CB_GETCURSEL,0,0));

		TempRightJS.DiDevice=(unsigned char)
			SendDlgItemMessage(hDlg,IDC_RIGHTJOYSTICKDEVICE,CB_GETCURSEL,0,0);
		TempLeftJS.DiDevice=(unsigned char)
			SendDlgItemMessage(hDlg,IDC_LEFTJOYSTICKDEVICE,CB_GETCURSEL,0,0);	//Fix Me;

		SendDlgItemMessage(hDlg,IDC_LEFTICON,
				STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[TempLeftJS.UseMouse]);
		SendDlgItemMessage(hDlg,IDC_RIGHTICON,
				STM_SETIMAGE ,(WPARAM)IMAGE_ICON,(LPARAM)JoystickIcons[TempRightJS.UseMouse]);
		break; // Break WM_COMMAND

	} //END switch message
	return(0);
}

void SwapJoySticks()
{
	JoyStick TempJS;
	TempJS = LeftJS;
	LeftJS = RightJS;
	RightJS = TempJS;
	SetStickNumbers(LeftJS.DiDevice,RightJS.DiDevice);
	UpdateConfig();
	if (hJoyStickDlg) SendMessage(hJoyStickDlg,WM_INITDIALOG,0,0);
}

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

void RefreshJoystickStatus(void)
{
	unsigned char Index=0;
	bool Temp=false;

	NumberofJoysticks = EnumerateJoysticks();
	for (Index=0;Index<NumberofJoysticks;Index++) Temp=InitJoyStick (Index);

	if (RightJS.DiDevice>(NumberofJoysticks-1)) RightJS.DiDevice=0;
	if (LeftJS.DiDevice>(NumberofJoysticks-1)) LeftJS.DiDevice=0;
	SetStickNumbers(LeftJS.DiDevice,RightJS.DiDevice);
	//Use Mouse input if no Joysticks present
	if (NumberofJoysticks==0) {
		if (LeftJS.UseMouse==3) LeftJS.UseMouse=1;
		if (RightJS.UseMouse==3) RightJS.UseMouse=1;
	}
	return;
}

/********************************************/
/*             BitBanger Config             */
/********************************************/
static char TextMode=1,PrtMon=0;;

HWND hBitBangerDlg = NULL;
void OpenBitBangerConfig() {
	if (hBitBangerDlg==NULL) {
		hBitBangerDlg = CreateDialog
			( EmuState.WindowInstance, (LPCTSTR) IDD_BITBANGER,
			  EmuState.WindowHandle,   (DLGPROC) BitBanger );
	}
	ShowWindow(hBitBangerDlg,SW_SHOWNORMAL);
	SetFocus(hBitBangerDlg);
}
LRESULT CALLBACK BitBanger(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_INITDIALOG: //IDC_PRINTMON
		if (!strlen(SerialCaptureFile))
			strcpy(SerialCaptureFile,"No Capture File");
		SendDlgItemMessage(hDlg,IDC_SERIALFILE,
			WM_SETTEXT,0,(LPARAM)(LPCSTR)SerialCaptureFile);
		SendDlgItemMessage(hDlg,IDC_LF,BM_SETCHECK,TextMode,0);
		SendDlgItemMessage(hDlg,IDC_PRINTMON,BM_SETCHECK,PrtMon,0);
		SetSerialParams(TextMode);
		SetMonState(PrtMon);
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDCANCEL:
		case IDCLOSE:
			hBitBangerDlg = NULL;
			DestroyWindow(hDlg);
			break;
		case IDOK:
		case IDAPPLY:
			UpdateConfig();
			if (LOWORD(wParam)==IDOK) {
				hBitBangerDlg = NULL;
				DestroyWindow(hDlg);
			}
			break;

		case IDC_OPEN:
			SelectFile(SerialCaptureFile);
			SendDlgItemMessage(hDlg,IDC_SERIALFILE,
					WM_SETTEXT,0,(LPARAM)(LPCSTR)SerialCaptureFile);
			break;

		case IDC_CLOSE:
			ClosePrintFile();
			strcpy(SerialCaptureFile,"No Capture File");
			SendDlgItemMessage(hDlg,IDC_SERIALFILE,
					WM_SETTEXT,0,(LPARAM)(LPCSTR)SerialCaptureFile);
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

		}	   //End switch wParam
		break; //End WM_COMMAND
	}  //End switch message
	return(0);
}

/********************************************/
/*               Paths Config               */
/********************************************/
LRESULT CALLBACK Paths(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	return 1;
}

/********************************************/
/*             Select Config File           */
/********************************************/
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
	ofn.lpstrDefExt       = "txt";
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
		if (strcmp(CapFilePath, "") != 0) {
			WritePrivateProfileString("DefaultPaths", "CapFilePath", CapFilePath, IniFilePath);
		}
	}
	strcpy(FileName,TempFileName);

	return(1);
}

/**********************************/
/*          Misc Exports          */
/**********************************/

// tcc1014graphics.c
int GetPaletteType()
{
	return(CurrentConfig.PaletteType);
}

// Called by DirectDrawInterface.cpp
int GetRememberSize()
{
	return CurrentConfig.RememberSize;
}

// Called by DirectDrawInterface.cpp
const Rect &GetIniWindowRect()
{
	return CurrentConfig.WindowRect;
}

void CaptureCurrentWindowRect()
{
	CurrentConfig.WindowRect = GetCurWindowSize();
}

//Called by tcc1014mmu:LoadRom
void GetExtRomPath(char * RomPath)
{
	if (CurrentConfig.UseExtCocoRom)
		strncpy(RomPath,CurrentConfig.ExtRomFile,MAX_PATH);
	else
		*RomPath = '\0';;
}
