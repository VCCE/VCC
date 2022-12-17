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

/*---------------------------------------------------------------

---------------------------------------------------------------*/
//#define STRICT
//#define WIN32_LEAN_AND_MEAN
/*--------------------------------------------------------------------------*/

#define DIRECTINPUT_VERSION 0x0800
#define _WIN32_WINNT 0x0500
#ifndef ABOVE_NORMAL_PRIORITY_CLASS
//#define ABOVE_NORMAL_PRIORITY_CLASS  32768
#endif

#define TH_RUNNING	0
#define TH_REQWAIT	1
#define TH_WAITING	2

#include <objbase.h>
#include <windowsx.h>
#include <process.h>
#include <commdlg.h>
#include <stdio.h>
#include <Mmsystem.h>
#include "fileops.h"
#include "defines.h"
#include "resource.h"
#include "joystickinput.h"
#include "Vcc.h"
#include "tcc1014mmu.h"
#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "hd6309.h"
#include "mc6809.h"
#include "mc6821.h"
#include "keyboard.h"
#include "coco3.h"
#include "pakinterface.h"
#include "audio.h"
#include "config.h"
#include "quickload.h"
#include "throttle.h"
#include "DirectDrawInterface.h"

#include "CommandLine.h"
#include "logger.h"

#include "MemoryMap.h"
#include "ProcessorState.h"
#include "Breakpoints.h"
#include "MMUMonitor.h"
#include "ExecutionTrace.h"

static HANDLE hout=NULL;

SystemState EmuState;
static bool DialogOpen=false;
static unsigned char Throttle=0;
static unsigned char AutoStart=1;
static unsigned char Qflag=0;
static char CpuName[20]="CPUNAME";

char QuickLoadFile[256];

/***Forward declarations of functions included in this code module*****/
BOOL				InitInstance	(HINSTANCE, int);
LRESULT CALLBACK	About			(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam );

void SoftReset(void);
void LoadIniFile(void);
void SaveConfig(void);
unsigned __stdcall EmuLoop(void *);
unsigned __stdcall CartLoad(void *);
void (*CPUInit)(void)=NULL;
int  (*CPUExec)( int)=NULL;
void (*CPUReset)(void)=NULL;
void (*CPUSetBreakpoints)(const std::vector<unsigned short>&) = NULL;
void (*CPUSetTraceTriggers)(const std::vector<unsigned short>&) = NULL;
VCC::CPUState (*CPUGetState)() = NULL;
void (*CPUAssertInterupt)(unsigned char,unsigned char)=NULL;
void (*CPUDeAssertInterupt)(unsigned char)=NULL;
void (*CPUForcePC)(unsigned short)=NULL;
void FullScreenToggle(void);
void save_key_down(unsigned char kb_char, unsigned char OEMscan);
void raise_saved_keys(void);
void PauseUnPause_Emulation(void);

// Message handlers
//void OnDestroy(HWND hwnd);
//void OnCommand(HWND hWnd, int iID, HWND hwndCtl, UINT uNotifyCode);
//void OnPaint(HWND hwnd);
// Globals
static 	HANDLE hEMUThread ;
static	HANDLE hEMUQuit;

static char g_szAppName[MAX_LOADSTRING] = "";
static bool emu_paused = false;
bool BinaryRunning;
static unsigned char FlagEmuStop=TH_RUNNING;
//static CRITICAL_SECTION  FrameRender;
/*--------------------------------------------------------------------------*/
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	MSG  Msg;

	EmuState.WindowInstance = hInstance;
	char temp1[MAX_PATH]="";
	char temp2[MAX_PATH]=" Running on ";
	unsigned threadID;
	HANDLE hEvent,

//	SetPriorityClass(GetCurrentProcess(),ABOVE_NORMAL_PRIORITY_CLASS );
//	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
//	CoInitializeEx(NULL,COINIT_MULTITHREADED);
//	CoInitialize(NULL);
//	InitializeCriticalSection();
	OleInitialize(NULL); //Work around fixs app crashing in "Open file" system dialogs (related to Adobe acrobat 7+
	LoadString(hInstance, IDS_APP_TITLE,g_szAppName, MAX_LOADSTRING);

	GetCmdLineArgs(lpCmdLine);                   //Parse command line

	if ( strlen(CmdArg.QLoadFile) !=0)
	{
		strcpy(QuickLoadFile, CmdArg.QLoadFile);
		strcpy(temp1, CmdArg.QLoadFile);
		PathStripPath(temp1);
		_strlwr(temp1);
		temp1[0]=toupper(temp1[0]);
		strcat (temp1,temp2);
		strcat(temp1,g_szAppName);
		strcpy(g_szAppName,temp1);
	}

	EmuState.WindowSize.x=640;
	EmuState.WindowSize.y=480;
	//LoadConfig(&EmuState);
	InitInstance(hInstance, nCmdShow);
	if (!CreateDDWindow(&EmuState))
	{
		MessageBox(0,"Can't create primary Window","Error",0);
		exit(0);
	}

	Cls(0,&EmuState);
	DynamicMenuCallback( "",0, 0);
	DynamicMenuCallback( "",1, 0);
	LoadConfig(&EmuState);			//Loads the default config file Vcc.ini from the exec directory
	EmuState.ResetPending=2;
	SetClockSpeed(1);	//Default clock speed .89 MHZ	
	BinaryRunning = true;
	EmuState.EmulationRunning=AutoStart;
	if (strlen(CmdArg.QLoadFile)!=0)
	{
		Qflag=255;
		EmuState.EmulationRunning=1;
	}
	hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ) ;
	if (hEvent==NULL)
	{
		MessageBox(0,"Can't create Thread!!","Error",0);
		return(0);
	}
	hEMUQuit = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEMUQuit == NULL)
	{
		MessageBox(0, "Can't create Thread Quit Event!!", "Error", 0);
		return(0);
	}
	hEMUThread = (HANDLE)_beginthreadex( NULL, 0, &EmuLoop, hEvent, 0, &threadID );
	if (hEMUThread==NULL)
	{
		MessageBox(0,"Can't Start main Emulation Thread!","Ok",0);
		return(0);
	}
	WaitForSingleObject( hEvent, INFINITE );
	SetThreadPriority(hEMUThread,THREAD_PRIORITY_NORMAL);

	//InitializeCriticalSection(&FrameRender);

	while (BinaryRunning)
	{
		if (FlagEmuStop==TH_WAITING)		//Need to stop the EMU thread for screen mode change
			{								//As it holds the Secondary screen buffer open while running
				FullScreenToggle();
				FlagEmuStop=TH_RUNNING;
			}
			GetMessage(&Msg,NULL,0,0);		//Seems if the main loop stops polling for Messages the child threads stall
			TranslateMessage(&Msg);
			DispatchMessage(&Msg) ;
	}

	SetEvent(hEMUQuit);								// Signal emulation thread to finish up.
	WaitForSingleObject(hEMUThread, INFINITE);		// Wait for emulation thread to terminate
	CloseHandle( hEvent ) ;	
	CloseHandle( hEMUQuit ) ;
	CloseHandle( hEMUThread ) ;
	timeEndPeriod(1);
	UnloadDll();
	SoundDeInit();
	WriteIniFile(); //Save Any changes to ini File
	return Msg.wParam;
}

/*--------------------------------------------------------------------------*/
// The Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	unsigned char kb_char;
	static unsigned char OEMscan=0;
    int Extended;
	static char ascii=0;
	static unsigned long Width,Height;

	kb_char = (unsigned char)wParam;

	switch (message)
	{

		case WM_SYSCOMMAND:
			//-------------------------------------------------------------
			// Control ATL key menu access.
			// Here left ALT is hardcoded off and right ALT on
			// TODO: Add config check boxes to control them
			//-------------------------------------------------------------
			if(wParam==SC_KEYMENU) {
				if (GetKeyState(VK_LMENU) & 0x8000) return 0; // Left off
			//	if (GetKeyState(VK_RMENU) & 0x8000) return 0; // Right off
			}
			// falls through to WM_COMMAND

		case WM_COMMAND:
			// Force all keys up to prevent key repeats
			raise_saved_keys();
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			// Parse the menu selections:
			// Added for Dynamic menu system
			if ( (wmId >=ID_SDYNAMENU) & (wmId <=ID_EDYNAMENU) )
			{
				DynamicMenuActivated (wmId - ID_SDYNAMENU);	//Calls to the loaded DLL so it can do the right thing
				break;
			}

			switch (wmId)
			{	
				case IDM_HELP_ABOUT:
					DialogBox(EmuState.WindowInstance,
						(LPCTSTR)IDD_ABOUTBOX,
						hWnd,
						(DLGPROC)About);
				    break;

				case ID_CONFIGURE_OPTIONS:				
#ifdef CONFIG_DIALOG_MODAL
					// open config dialog modally
					DialogBox(EmuState.WindowInstance,
						(LPCTSTR)IDD_TCONFIG,
						hWnd,
						(DLGPROC)Config
						);
#else
					// open config dialog if not already open
					// opens modeless so you can control the cassette
					// while emulator is still running (assumed)
					if (EmuState.ConfigDialog==NULL)
					{
						EmuState.ConfigDialog = CreateDialog(
							EmuState.WindowInstance, //NULL,
							(LPCTSTR)IDD_TCONFIG,
							EmuState.WindowHandle,
							(DLGPROC)Config
						) ;
						// open modeless
						ShowWindow(EmuState.ConfigDialog, SW_SHOWNORMAL) ;
					}
#endif
				    break;


				case IDOK:
					SendMessage (hWnd, WM_CLOSE, 0, 0);
					break;

				case ID_FILE_EXIT:
					BinaryRunning=0;
					break;

				case ID_FILE_RESET:
					if (EmuState.EmulationRunning)
						EmuState.ResetPending=2;
					break;

				case ID_FILE_RUN:
					EmuState.EmulationRunning=TRUE;
					InvalidateBoarder();
					break;

				case ID_FILE_RESET_SFT:
					if (EmuState.EmulationRunning)
						EmuState.ResetPending=1;
					break;

				case ID_FILE_LOAD:
					LoadIniFile();
					break;

                case ID_SAVE_CONFIG:
                    SaveConfig();
                    break;

				case ID_COPY_TEXT:
					CopyText();
					break;

				case ID_PASTE_TEXT:
					PasteText();
					break;

				case ID_PASTE_BASIC:
					PasteBASIC();
					break;

				case ID_PASTE_BASIC_NEW:
					PasteBASICWithNew();
					break;

				case ID_FLIP_ARTIFACTS:
					FlipArtifacts();
					break;

				case ID_PAUSE_EMULATION:
					PauseUnPause_Emulation();
					break;
				case ID_MEMORY_DISPLAY:
					VCC::Debugger::UI::OpenMemoryMapWindow(EmuState.WindowInstance, EmuState.WindowHandle);
				    break;
	
				case ID_PROCESSOR_STATE:
					VCC::Debugger::UI::OpenProcessorStateWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				case ID_BREAKPOINTS:
					VCC::Debugger::UI::OpenBreakpointsWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				case ID_MMU_MONITOR:
					VCC::Debugger::UI::OpenMMUMonitorWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				case ID_EXEC_TRACE:
					VCC::Debugger::UI::OpenExecutionTraceWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

//		case WM_CREATE:
//			break;

		case WM_SETCURSOR:
			// Hide mouse cursor
            if ((EmuState.MousePointer != 1) && (LOWORD(lParam) == HTCLIENT)) {
				SetCursor(NULL);
				return TRUE;
			}
			break;

		case WM_KILLFOCUS:
			// Force keys up if main widow keyboard focus is lost.  Otherwise
			// down keys will cause issues with OS-9 on return
			raise_saved_keys();
			break;

		case WM_CLOSE:
			BinaryRunning=0;
			break;

		case WM_CHAR:
//			OEMscan=(unsigned char)((lParam & 0xFF0000)>>16);
//			ascii=kb_char;
//			sprintf(ttbuff,"Getting REAL CHAR %i",ascii);
//			WriteLine ( ttbuff);
//			KeyboardEvent(kb_char,OEMscan,1); //Capture ascii value for scancode
			return 0;
			break;

		case WM_SYSCHAR:
			DefWindowProc(hWnd, message, wParam, lParam);
			return 0;
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:

			// send emulator key up event to the emulator
			// TODO: Key up checks whether the emulation is running, this does not

			//OEMscan = (unsigned char)((lParam & 0x00FF0000)>>16);
	        OEMscan = (unsigned char) ((lParam >> 16) & 0xFF);
            Extended=(lParam >> 24) & 1;
		    if (Extended && (OEMscan!=DIK_NUMLOCK)) OEMscan += 0x80;
			vccKeyboardHandleKey(kb_char,OEMscan,kEventKeyUp);
			
			return 0;
			break;

//----------------------------------------------------------------------------------------
//	lParam bits
//	  0-15	The repeat count for the current message. The value is the number of times
//			the keystroke is autorepeated as a result of the user holding down the key.
//			If the keystroke is held long enough, multiple messages are sent. However,
//			the repeat count is not cumulative.
//	 16-23	The scan code. The value depends on the OEM.
//	    24	Indicates whether the key is an extended key, such as the right-hand ALT and
//			CTRL keys that appear on an enhanced 101- or 102-key keyboard. The value is
//			one if it is an extended key; otherwise, it is zero.
//	 25-28	Reserved; do not use.
//	    29	The context code. The value is always zero for a WM_KEYDOWN message.
//	    30	The previous key state. The value is one if the key is down before the
//	   		message is sent, or it is zero if the key is up.
//	    31	The transition state. The value is always zero for a WM_KEYDOWN message.
//----------------------------------------------------------------------------------------

		case WM_SYSKEYDOWN:

			// Ignore repeated system keys
			if (lParam>>30) return 0;

		case WM_KEYDOWN:

			// get key scan code for emulator control keys
			//OEMscan = (unsigned char)((lParam & 0x00FF0000)>>16); // just get the scan code
			OEMscan = (unsigned char) ((lParam >> 16) & 0xFF);
			Extended=(lParam >> 24) & 1;
			if (Extended && (OEMscan!=DIK_NUMLOCK)) OEMscan += 0x80;

			// kb_char = Windows virtual key code

			switch ( OEMscan )
			{
				case DIK_F3:
					DecreaseOverclockSpeed();
				break;

				case DIK_F4:
					IncreaseOverclockSpeed();
				break;

				case DIK_F5:
					if ( EmuState.EmulationRunning )
					{
						EmuState.ResetPending = 1;
					}
				break;

				case DIK_F6:
					SetMonitorType(!SetMonitorType(QUERY));
				break;

//				case DIK_F7:
//					SetArtifacts(!SetArtifacts(QUERY));
//				break;

				case DIK_F8:
					if (!IsShiftKeyDown()) SetSpeedThrottle(!SetSpeedThrottle(QUERY));
					else PauseUnPause_Emulation();
				break;

				case DIK_F9:
					EmuState.EmulationRunning=!EmuState.EmulationRunning;
					if ( EmuState.EmulationRunning )
						EmuState.ResetPending=2;
					else
						SetStatusBarText("",&EmuState);
				break;

				case DIK_F10:
					SetInfoBand(!SetInfoBand(QUERY));
					InvalidateBoarder();
				break;
				
				case DIK_F11:
					if (FlagEmuStop == TH_RUNNING)
					{
						FlagEmuStop = TH_REQWAIT;
						EmuState.FullScreen =! EmuState.FullScreen;
					}
		
				break;

//				case DIK_F12:
//					CpuDump();
//				break;

				default:
					// send other keystrokes to the emulator if it is active
					if ( EmuState.EmulationRunning )
					{
						vccKeyboardHandleKey(kb_char, OEMscan, kEventKeyDown);
						// Save key down in case focus is lost
						save_key_down(kb_char,OEMscan);
					}
					break;
			}
			return 0;
			break;

		case WM_LBUTTONDOWN:  //0 = Left 1=right
			SetButtonStatus(0,1);
			break;

		case WM_LBUTTONUP:
			SetButtonStatus(0,0);
			break;

		case WM_RBUTTONDOWN:
			SetButtonStatus(1,1);
			break;

		case WM_RBUTTONUP:
			SetButtonStatus(1,0);
			break;

		case WM_MOUSEMOVE:
			if (EmuState.EmulationRunning)
			{
				static const float MAX_AXIS_VALUE = 16384.0f;

				//	Get the dimensions of the usable client area (i.e. sans status bar)
				RECT clientRect;
				GetClientRect(EmuState.WindowHandle, &clientRect);
				clientRect.bottom -= GetRenderWindowStatusBarHeight();

				int mouseXPosition = GET_X_LPARAM(lParam);
				int mouseYPosition = GET_Y_LPARAM(lParam);
				int maxHorizontalPosition = clientRect.right;
				int maxVerticalPosition = clientRect.bottom;

				if (!EmuState.FullScreen)
				{
					const DisplayDetails displayDetails(GetDisplayDetails(clientRect.right, clientRect.bottom));
					
					maxHorizontalPosition -= (displayDetails.leftBorderColumns + displayDetails.rightBorderColumns);
					maxVerticalPosition -= (displayDetails.topBorderRows + displayDetails.bottomBorderRows);

					mouseXPosition = min(
						max(0, mouseXPosition - displayDetails.leftBorderColumns),
						maxHorizontalPosition);
					mouseYPosition = min(
						max(0, mouseYPosition - displayDetails.topBorderRows),
						maxVerticalPosition);
				}

				// Convert coordinates to mouseXPosition,mouseYPosition values with range 0-3fff (0-16383).
				mouseXPosition = static_cast<int>(mouseXPosition * (MAX_AXIS_VALUE / maxHorizontalPosition));
				mouseYPosition = static_cast<int>(mouseYPosition * (MAX_AXIS_VALUE / maxVerticalPosition));

				joystick(mouseXPosition, mouseYPosition);
			}

			return 0;

//		default:
//			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

//--------------------------------------------------------------------------
// When the main window is about to lose keyboard focus there are one
// or two keys down in the emulation that must be raised.  These routines
// track the last two key down events so they can be raised when needed.
//--------------------------------------------------------------------------

static unsigned char SC_save1 = 0;
static unsigned char SC_save2 = 0;
static unsigned char KB_save1 = 0;
static unsigned char KB_save2 = 0;
static int KeySaveToggle=0;

// Save last two key down events
void save_key_down(unsigned char kb_char, unsigned char OEMscan) {

	// Ignore zero scan code
	if (OEMscan == 0) return;

	// Remember it
	KeySaveToggle = !KeySaveToggle;
	if (KeySaveToggle) {
		KB_save1 = kb_char;
		SC_save1 = OEMscan;
	} else {
		KB_save2 = kb_char;
		SC_save2 = OEMscan;
	}
	return;
}

// Send key up events to keyboard handler for saved keys
void raise_saved_keys() {
	if (SC_save1) vccKeyboardHandleKey(KB_save1,SC_save1,kEventKeyUp);
	if (SC_save2) vccKeyboardHandleKey(KB_save2,SC_save2,kEventKeyUp);
	SC_save1 = 0;
	SC_save2 = 0;
	return;
}

/*--------------------------------------------------------------------------
// Command message handler
void OnCommand(HWND hWnd, int iID, HWND hwndCtl, UINT uNotifyCode)
{
	//switch (iID)
	//{
	//case IDM_QUIT:
	//	OnDestroy(hWnd);
	//	break;
	//}
}
--------------------------------------------------------------------------*/
// Handle WM_DESTROY
void OnDestroy(HWND )
{
	BinaryRunning = false;
	PostQuitMessage(0);
}
/*--------------------------------------------------------------------------*/
// Window painting function
void OnPaint(HWND hwnd)
{
	// Let Windows know we've redrawn the Window - since we've bypassed
	// the GDI, Windows can't figure that out by itself.
//	ValidateRect( hwnd, NULL );
	
	// Over here we could do some normal GDI drawing
//	PAINTSTRUCT ps;
//	HDC hdc;
//	HDC hdc = BeginPaint(hwnd, &ps);
//	if (hdc)
//	{
//	}
//	EndPaint(hwnd, &ps);
}
/*--------------------------------------------------------------------------*/

void SetCPUMultiplyerFlag (unsigned char double_speed)
{
	SetClockSpeed(1);
	EmuState.DoubleSpeedFlag=double_speed;
	if (EmuState.DoubleSpeedFlag)
		SetClockSpeed( EmuState.DoubleSpeedMultiplyer * EmuState.TurboSpeedFlag);
	EmuState.CPUCurrentSpeed= .894;
	if (EmuState.DoubleSpeedFlag)
		EmuState.CPUCurrentSpeed*=(EmuState.DoubleSpeedMultiplyer*EmuState.TurboSpeedFlag);
	return;
}

void SetTurboMode(unsigned char data)
{
	EmuState.TurboSpeedFlag=(data&1)+1;
	SetClockSpeed(1);
	if (EmuState.DoubleSpeedFlag)
		SetClockSpeed( EmuState.DoubleSpeedMultiplyer * EmuState.TurboSpeedFlag);
	EmuState.CPUCurrentSpeed= .894;
	if (EmuState.DoubleSpeedFlag)
		EmuState.CPUCurrentSpeed*=(EmuState.DoubleSpeedMultiplyer*EmuState.TurboSpeedFlag);
	return;
}

unsigned char SetCPUMultiplyer(unsigned char Multiplyer)
{
	if (Multiplyer!=QUERY)
	{
		EmuState.DoubleSpeedMultiplyer=Multiplyer;
		SetCPUMultiplyerFlag (EmuState.DoubleSpeedFlag);
	}
	return(EmuState.DoubleSpeedMultiplyer);
}

void DoHardReset(SystemState* const HRState)
{	
	HRState->RamBuffer=MmuInit(HRState->RamSize);	//Alocate RAM/ROM & copy ROM Images from source
	HRState->WRamBuffer=(unsigned short *)HRState->RamBuffer;

	// Debugger - watch memory.
	// Only Processor Space (64K) is watched at the moment.
	HRState->Debugger.Reset();
	// Debugger ---------------------

	if (HRState->RamBuffer == NULL)
	{
		MessageBox(NULL,"Can't allocate enough RAM, Out of memory","Error",0);
		exit(0);
	}
	if (HRState->CpuType == 1)
	{
		CPUInit = HD6309Init;
		CPUExec = HD6309Exec;
		CPUReset = HD6309Reset;
		CPUAssertInterupt = HD6309AssertInterupt;
		CPUDeAssertInterupt = HD6309DeAssertInterupt;
		CPUForcePC = HD6309ForcePC;
		CPUSetBreakpoints = HD6309SetBreakpoints;
		CPUGetState = HD6309GetState;
		CPUSetTraceTriggers = HD6309SetTraceTriggers;
	}
	else
	{
		CPUInit = MC6809Init;
		CPUExec = MC6809Exec;
		CPUReset = MC6809Reset;
		CPUAssertInterupt = MC6809AssertInterupt;
		CPUDeAssertInterupt = MC6809DeAssertInterupt;
		CPUForcePC = MC6809ForcePC;
		CPUSetBreakpoints = MC6809SetBreakpoints;
		CPUGetState = MC6809GetState;
		CPUSetTraceTriggers = MC6809SetTraceTriggers;
	}
	PiaReset();
	mc6883_reset();	//Captures interal rom pointer for CPU Interupt Vectors
	CPUInit();
	CPUReset();		// Zero all CPU Registers and sets the PC to VRESET
	GimeReset();
	UpdateBusPointer();
	EmuState.TurboSpeedFlag=1;
	ResetBus();
	SetClockSpeed(1);
	return;
}

void SoftReset(void)
{
	mc6883_reset();
	PiaReset();
	CPUReset();
	GimeReset();
	MmuReset();
	CopyRom();
	ResetBus();
	EmuState.TurboSpeedFlag=1;
	return;
}

// Mesage handler for the About box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_TITLE,WM_SETTEXT,strlen(g_szAppName),(LPARAM)(LPCSTR)g_szAppName);		
			return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return TRUE;
			}
			break;
	}
    return FALSE;
}

unsigned char SetRamSize(unsigned char Size)
{
	if (Size!=QUERY)
		EmuState.RamSize=Size;
	return(EmuState.RamSize);
}

unsigned char SetSpeedThrottle(unsigned char throttle)
{
	if (throttle!=QUERY)
		Throttle=throttle;
	return(Throttle);
}

unsigned char SetFrameSkip(unsigned char Skip)
{
	if (Skip!=QUERY)
		EmuState.FrameSkip=Skip;
	return(EmuState.FrameSkip);
}

unsigned char SetCpuType( unsigned char Tmp)
{
	switch (Tmp)
	{
	case 0:
		EmuState.CpuType=0;
		strcpy(CpuName,"MC6809");
		break;

	case 1:
		EmuState.CpuType=1;
		strcpy(CpuName,"HD6309");
		break;
	}
	return(EmuState.CpuType);
}

void DoReboot(void)
{
	EmuState.ResetPending=2;
	return;
}

unsigned char SetAutoStart(unsigned char Tmp)
{
	if (Tmp != QUERY)
		AutoStart=Tmp;
	return(AutoStart);
}

// LoadIniFile allows user to browse for an ini file and reloads the config from it.
void LoadIniFile(void)
{
    OPENFILENAME ofn ;	
    char szFileName[MAX_PATH]="";

    GetIniFilePath(szFileName); // EJJ load current ini file path

    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize       = sizeof (OPENFILENAME);
    ofn.hwndOwner         = EmuState.WindowHandle;
    ofn.lpstrFilter       = "INI\0*.ini\0\0";
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFileName;
    ofn.nMaxFile          = MAX_PATH;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrFileTitle    = NULL;
    ofn.lpstrInitialDir   = AppDirectory() ;
    ofn.lpstrTitle        = TEXT("Load Vcc Config File") ;
    ofn.Flags             = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

    if ( GetOpenFileName (&ofn) ) {
        WriteIniFile();               // Flush current profile
        SetIniFilePath(szFileName);   // Set new ini file path
        ReadIniFile();                // Load it
        UpdateConfig();
        EmuState.ResetPending = 2;
//      SetClockSpeed(1); //Default clock speed .89 MHZ
    }
    return;
}

// SaveIniFile copies the current config to a speficied ini file. The file is created
// if it does not already exist.

void SaveConfig(void) {
    OPENFILENAME ofn ;
    char curini[MAX_PATH];
    char newini[MAX_PATH+4];  // Save room for '.ini' if needed

    GetIniFilePath(curini);  // EJJ get current ini file path
    strcpy(newini,curini);   // Let GetOpenFilename suggest it

    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize       = sizeof (OPENFILENAME) ;
    ofn.hwndOwner         = EmuState.WindowHandle;
    ofn.lpstrFilter       = "INI\0*.ini\0\0" ;        // filter string
    ofn.nFilterIndex      = 1 ;                       // current filter index
    ofn.lpstrFile         = newini;                   // contains full path on return
    ofn.nMaxFile          = MAX_PATH;                 // sizeof lpstrFile
    ofn.lpstrFileTitle    = NULL;                     // filename and extension only
    ofn.nMaxFileTitle     = MAX_PATH ;                // sizeof lpstrFileTitle
    ofn.lpstrInitialDir   = AppDirectory() ;          // EJJ initial directory
    ofn.lpstrTitle        = TEXT("Save Vcc Config") ; // title bar string
    ofn.Flags             = OFN_HIDEREADONLY |OFN_PATHMUSTEXIST;

    if ( GetOpenFileName (&ofn) ) {
        if (ofn.nFileExtension == 0) strcat(newini, ".ini");  //Add extension if none
        WriteIniFile();                                       // Flush current config
        if (_stricmp(curini,newini) != 0) {
		if (! CopyFile(curini,newini,false) ) {       // Copy it to new file
			MessageBox(0,"Copy config failed","error",0);
		}
        }
    }
    return;
}

unsigned __stdcall EmuLoop(void *Dummy)
{
	HANDLE hEvent = (HANDLE)Dummy;
	static float FPS;
	static unsigned int FrameCounter=0;	
	CalibrateThrottle();
	Sleep(30);
	SetEvent(hEvent) ;

	while (true)
	{
		// Main Thread wants us to end?
		if (WaitForSingleObject(hEMUQuit,0) == WAIT_OBJECT_0)
		{
			break;
		}
		if (FlagEmuStop==TH_REQWAIT)
		{
			FlagEmuStop = TH_WAITING; //Signal Main thread we are waiting
			while (FlagEmuStop == TH_WAITING)
				Sleep(1);
		}
		FPS=0;
		if ((Qflag==255) & (FrameCounter==30))
		{
			Qflag=0;
			QuickLoad(QuickLoadFile);
		}

		StartRender();
		for (uint8_t Frames = 1; Frames <= EmuState.FrameSkip; Frames++)
		{
			FrameCounter++;
			if (EmuState.ResetPending != 0) {
				switch (EmuState.ResetPending)
				{
				case 1:	//Soft Reset
					SoftReset();
					break;

				case 2:	//Hard Reset
					UpdateConfig();
					DoCls(&EmuState);
					DoHardReset(&EmuState);
					break;

				case 3:
					DoCls(&EmuState);
					break;

				case 4:
					UpdateConfig();
					DoCls(&EmuState);
					break;

				default:
					break;
				}
				EmuState.ResetPending = 0;
			}
			if (EmuState.EmulationRunning == 1 && !emu_paused) {
				FPS += RenderFrame(&EmuState);
			} else if(!emu_paused) {
				FPS += Static(&EmuState);
			}
		}
		EndRender(EmuState.FrameSkip);
		FPS/=EmuState.FrameSkip;
		GetModuleStatus(&EmuState);
		
		char ttbuff[256];
		snprintf(ttbuff,sizeof(ttbuff),"Skip:%2.2i | FPS:%3.0f | %s @ %2.2fMhz | %s",EmuState.FrameSkip,FPS,CpuName,EmuState.CPUCurrentSpeed,EmuState.StatusLine);
		if(!emu_paused) SetStatusBarText(ttbuff,&EmuState);
		else SetStatusBarText("Emulation Paused. Press SHIFT-F8 to continue.", &EmuState);
		
		if (Throttle )	//Do nothing untill the frame is over returning unused time to OS
			FrameWait();
	} //Still Emulating

	return(NULL);
}

void LoadPack(void)
{
	unsigned threadID;
	if (DialogOpen)
		return;
	DialogOpen=true;
	_beginthreadex( NULL, 0, &CartLoad, CreateEvent( NULL, FALSE, FALSE, NULL ), 0, &threadID );
}

unsigned __stdcall CartLoad(void *Dummy)
{
	LoadCart();
	EmuState.EmulationRunning=TRUE;
	DialogOpen=false;
	return(NULL);
}

void FullScreenToggle(void)
{
	PauseAudio(true);
	if (!CreateDDWindow(&EmuState))
	{
		MessageBox(0,"Can't rebuild primary Window","Error",0);
		exit(0);
	}
	InvalidateBoarder();
	RefreshDynamicMenu();
	EmuState.ConfigDialog=NULL;
	PauseAudio(false);
	return;
}
void PauseUnPause_Emulation() {
	// User selected 'Pause' from the menu. Pause the emulation and 
	// temporarily disable the sound in case it was paused during a sample.
	if (!emu_paused) {
		emu_paused = true;
		PauseAudio(1);
		OutputDebugString("Emulation paused\n");
	}
	else {
		PauseAudio(0);
		emu_paused = false;
		OutputDebugString("Emulation unpaused\n");
	}
	return;
}


