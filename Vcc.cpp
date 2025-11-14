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

// FIXME: This should be defined on the command line
#define DIRECTINPUT_VERSION 0x0800
#define _WIN32_WINNT 0x0500
#ifndef ABOVE_NORMAL_PRIORITY_CLASS
//#define ABOVE_NORMAL_PRIORITY_CLASS  32768
#endif

// FIXME: These defines need to be converted to a scoped enumeration
#define TH_RUNNING	0
#define TH_REQWAIT	1
#define TH_WAITING	2

#include "BuildConfig.h"
#include <objbase.h>
#include <windowsx.h>
#include <process.h>
#include <commdlg.h>
#include <stdio.h>
#include <mmsystem.h>
#include "vcc/ui/menu/menu_builder.h"
#include "vcc/utils/FileOps.h"
#include "vcc/common/DialogOps.h"
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
#include "Cassette.h"

#include "CommandLine.h"
#include "vcc/utils/logger.h"
#include "memdump.h"

#include "MemoryMap.h"
#include "ProcessorState.h"
#include "SourceDebug.h"
#include "Disassembler.h"
#include "MMUMonitor.h"
#include "ExecutionTrace.h"

#if USE_DEBUG_MOUSE
#include "IDisplayDebug.h"
#endif
#include "menu_populator.h"
#include <vcc/utils/cartridge_catalog.h>

using namespace VCC;

static HANDLE hout=nullptr;

SystemState EmuState;
static unsigned char AutoStart=1;
static unsigned char Qflag=0;
static unsigned char Pflag=0;
static char CpuName[20]="CPUNAME";

/***Forward declarations of functions included in this code module*****/
BOOL InitInstance (HINSTANCE, int);
BOOL CALLBACK About(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK FunctionKeys(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM);

void SoftReset();
void LoadIniFile();
void SaveConfig();
unsigned __stdcall EmuLoop(HANDLE hEvent);
void (*CPUInit)()=nullptr;
int  (*CPUExec)( int)=nullptr;
void (*CPUReset)()=nullptr;
void (*CPUSetBreakpoints)(const std::vector<unsigned short>&) = nullptr;
void (*CPUSetTraceTriggers)(const std::vector<unsigned short>&) = nullptr;
VCC::CPUState (*CPUGetState)() = nullptr;
void (*CPUAssertInterupt)(InterruptSource, Interrupt)=nullptr;
void (*CPUDeAssertInterupt)(InterruptSource, Interrupt)=nullptr;
void (*CPUForcePC)(unsigned short)=nullptr;
void FullScreenToggle();
void save_key_down(unsigned char kb_char, unsigned char OEMscan);
void raise_saved_keys();
void SetupClock();
HMENU GetConfMenu();
void CALLBACK update_status(HWND, UINT, UINT_PTR, DWORD);
static void InsertRomPakCartridge();
static void InsertCartridge(_beginthreadex_proc_type proc);

// Globals
static 	HANDLE hEMUThread ;
static	HANDLE hEMUQuit;

constexpr auto first_cartridge_menu_id = 5000;
constexpr auto last_cartridge_menu_id = 5999;

struct menu_bar_indexes
{
	static const auto control_menu = 3u;
	static const auto cartridge_menu = 4u;
};

constexpr auto first_select_cartridge_menu_id = 6000;
constexpr auto last_select_cartridge_menu_id = 6099;

// Function key overclocking flag
//unsigned char OverclockFlag = 1;

static char g_szAppName[MAX_LOADSTRING] = "";
bool BinaryRunning;
static unsigned char FlagEmuStop=TH_RUNNING;

bool IsShiftKeyDown();

static void ProcessSelectDeviceCartridgeMenuItem(const vcc::utils::cartridge_catalog::item_type& catalog_item);
static vcc::utils::cartridge_catalog::item_container_type cartridge_menu_catalog_items;
static void LoadStartupCartridge();

//static CRITICAL_SECTION  FrameRender;
/*--------------------------------------------------------------------------*/
int APIENTRY WinMain(_In_ HINSTANCE hInstance,
					  _In_opt_ HINSTANCE hPrevInstance,
					  _In_ LPSTR    lpCmdLine,
					  _In_ int       nCmdShow)
{
	MSG  Msg;

	EmuState.WindowInstance = hInstance;
	unsigned threadID;
	HANDLE hEvent,
	OleInitialize(nullptr); //Work around fixs app crashing in "Open file" system dialogs (related to Adobe acrobat 7+
	LoadString(hInstance, IDS_APP_TITLE,g_szAppName, MAX_LOADSTRING);

	// Parse command line
	memset(&CmdArg,0,sizeof(CmdArg));
	if (strlen(lpCmdLine)>0) GetCmdLineArgs(lpCmdLine);

	Cls(0,&EmuState);
	EmuState.Throttle = 1;
	EmuState.WindowSize.w=DefaultWidth;
	EmuState.WindowSize.h=DefaultHeight;
	EmuState.Exiting = false;
	LoadConfig(&EmuState);
	EmuState.ResetPending=2; // after LoadConfig pls
	InitInstance(hInstance, nCmdShow);
	if ((CmdArg.NoOutput && !CreateNullWindow(&EmuState)) ||
		(!CmdArg.NoOutput && !CreateDDWindow(&EmuState)))
	{
		MessageBox(nullptr,"Can't create primary Window","Error",0);
		exit(0);
	}

	InitSound();
	LoadStartupCartridge();
	SetClockSpeed(1);	//Default clock speed .89 MHZ	
	BinaryRunning = true;
	EmuState.EmulationRunning=AutoStart;

	if (strlen(CmdArg.PasteText)!=0) {
		Pflag=255;
	}

	if (strlen(CmdArg.QLoadFile)!=0)
	{
		Qflag=255;
		EmuState.EmulationRunning=1;
	}
	hEvent = CreateEvent( nullptr, FALSE, FALSE, nullptr ) ;
	if (hEvent==nullptr)
	{
		MessageBox(nullptr,"Can't create Thread!!","Error",0);
		return 0;
	}
	hEMUQuit = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (hEMUQuit == nullptr)
	{
		MessageBox(nullptr, "Can't create Thread Quit Event!!", "Error", 0);
		return 0;
	}
	hEMUThread = (HANDLE)_beginthreadex( nullptr, 0, &EmuLoop, hEvent, 0, &threadID );
	if (hEMUThread==nullptr)
	{
		MessageBox(nullptr,"Can't Start main Emulation Thread!","Ok",0);
		return 0;
	}
	WaitForSingleObject( hEvent, INFINITE );
	SetThreadPriority(hEMUThread,THREAD_PRIORITY_NORMAL);

	while (BinaryRunning)
	{
		if (FlagEmuStop==TH_WAITING)	//Need to stop the EMU thread for screen mode change
		{								//As it holds the Secondary screen buffer open while running
			FullScreenToggle();
			FlagEmuStop=TH_RUNNING;
		}
		if (CmdArg.NoOutput)
			Sleep(1);
		else
		{
			// Seems if the main loop stops polling the child threads stall
			// not GetMessage as it blocks, continue loop when no messages
			if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		}
	}
	EmuState.Exiting = true;

	PostThreadMessage(threadID, WM_QUIT, 0, 0);
	SetEvent(hEMUQuit);							// Signal emulation thread to finish up.

	// Wait for emulation thread to terminate
	while (WaitForSingleObject(hEMUThread, 10) == WAIT_TIMEOUT)
	{
		if (CmdArg.NoOutput)
			Sleep(1);
		else
		{
			// process messages while waiting, if any
			// not GetMessage as it blocks, continue loop when no messages
			if (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
		}
	}
	CloseHandle( hEvent ) ;	
	CloseHandle( hEMUQuit ) ;
	CloseHandle( hEMUThread ) ;
	CloseScreen();
	timeEndPeriod(1);
	PakEjectCartridge(false);
	WriteIniFile(); //Save Any changes to ini File
	return Msg.wParam;
}

void CloseApp()
{
	BinaryRunning = false;
	SoundDeInit();
	PakEjectCartridge(false);
}

void set_menu_item_text(HMENU menu, UINT id, std::string text, std::string hotkey)
{
	MENUITEMINFO item_info = {};

	if (!hotkey.empty())
	{
		text += "\t" + hotkey;
	}

	item_info.cbSize = sizeof(item_info);
	item_info.fMask = MIIM_STRING;
	item_info.fType = MFT_STRING;

	item_info.dwTypeData = text.data();
	item_info.cch = text.size();

	SetMenuItemInfo(menu, id, FALSE, &item_info);
}

void UpdateControlMenu(HMENU menu)
{
	set_menu_item_text(
		menu,
		ID_CONTROL_TOGGLE_PAUSE,
		EmuState.Debugger.IsHalted() ? "Resume" : "Pause",
		"F7");

	set_menu_item_text(
		menu,
		ID_CONTROL_TOGGLE_DISPLAY_TYPE,
		// FIXME-CHET: Magic number returned by setmonitortype should be enum
		// FIXME-CHET: QUERY SHIT!
		SetMonitorType(QUERY) == 0 ? "Switch to RGB Display" : "Switch to Composite Display",
		"F6");
}

[[nodiscard]] std::vector<vcc::utils::cartridge_catalog::item_type> UpdateCartridgeMenu(HMENU menu)
{
	while (GetMenuItemCount(menu) > 0)
	{
		RemoveMenu(menu, 0, MF_BYPOSITION);
	}

	::vcc::ui::menu::menu_builder builder;
	builder.add_root_submenu("Cartridge Port");
	if (const auto& name(PakGetName()); !name.empty())
	{
		// HACK: We subtract first_cartridge_menu_id from the id here because it gets 
		// added to each of the id's in the item list to virtualize them. Since this
		// id shouldn't be virtualized we hack it here to cheat,
		builder.add_submenu_item(ID_EJECT_CARTRIDGE - first_cartridge_menu_id, "Eject " + name);
		builder.add_submenu_separator();
	}

	builder.add_submenu_item(ID_INSERT_ROMPAK_CARTRIDGE - first_cartridge_menu_id, "Insert ROM Pak Catridge...");
	//builder.add_submenu_item(ID_CARTRIDGE_INSERT_ROM - first_cartridge_menu_id, "Recently used ROM Paks", nullptr, true);

	using item_type = vcc::utils::cartridge_catalog::item_type;

	const auto catalog_items(cartridge_catalog_.copy_items_ordered());
	
	if (!catalog_items.empty())
	{
		builder.add_submenu_separator();

		auto cartridgeSelectMenuItemId(first_select_cartridge_menu_id);
		for(auto& item : catalog_items)
		{
			bool disabled = cartridge_catalog_.is_loaded(item.id);
			// HACK: See above comment.
			builder.add_submenu_item(
				cartridgeSelectMenuItemId - first_cartridge_menu_id,
				"Insert " + item.name,
				{},
				disabled);

			++cartridgeSelectMenuItemId;
		}
	}

	if (const auto& menu_items(PakGetMenuItems()); !menu_items.empty())
	{
		builder.add_root_separator();
		builder.add_items(menu_items);
	}
	
	menu_populator menu_item_visitor(menu, first_cartridge_menu_id);

	builder
		.release_items()
		.accept(menu_item_visitor);

	return catalog_items;
}


/*--------------------------------------------------------------------------*/
// The Window Procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	unsigned char kb_char;
	static unsigned char OEMscan=0;
    int Extended;

	kb_char = (unsigned char)wParam;

	switch (message)
	{
		case WM_CREATE:
			SetTimer(hWnd, 100, 15, update_status);
			break;

		case WM_INITMENUPOPUP:
		{
			const auto menu(GetMenu(hWnd));
			
			if (const auto menu_opening(reinterpret_cast<HMENU>(wParam));
				menu_opening == GetSubMenu(menu, menu_bar_indexes::cartridge_menu))
			{
				cartridge_menu_catalog_items = UpdateCartridgeMenu(menu_opening);
			}
			else if (menu_opening == GetSubMenu(menu, menu_bar_indexes::control_menu))
			{
				UpdateControlMenu(menu_opening);
			}
			break;
		}

		case WM_SYSCOMMAND:
			//-------------------------------------------------------------
			// Control ATL key menu access.
			// Here left ALT is hardcoded off and right ALT on
			// TODO: Add config check boxes to control them
			//-------------------------------------------------------------
			if(wParam==SC_KEYMENU) {
				if (GetKeyState(VK_LMENU) & 0x8000) return 0; // Left off
			}
			// falls through to WM_COMMAND

		case WM_COMMAND:
			// Force all keys up to prevent key repeats
			raise_saved_keys();
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			// Check if ID is in cartridge menu range
			if (wmId >= first_cartridge_menu_id && wmId <= last_cartridge_menu_id)
			{
				CartMenuActivated(wmId - first_cartridge_menu_id);
				break;
			}

			if (wmId >= first_select_cartridge_menu_id && wmId <= last_select_cartridge_menu_id)
			{
				wmId -= first_select_cartridge_menu_id;
				if (static_cast<std::size_t>(wmId) < cartridge_menu_catalog_items.size())
				{
					ProcessSelectDeviceCartridgeMenuItem(cartridge_menu_catalog_items[wmId]);
				}

				break;
			}

			switch (wmId)
			{	
				case ID_INSERT_ROMPAK_CARTRIDGE:
					InsertRomPakCartridge();
					break;

				case ID_EJECT_CARTRIDGE:
					PakEjectCartridge(true);
					ClearStartupCartridge();
					break;


				case IDM_USER_WIKI:
					ShellExecute(nullptr, "open",
								 "https://github.com/VCCE/VCC/wiki/UserGuide",
								 nullptr, nullptr, SW_SHOWNORMAL);
					break;
				case IDM_HELP_ABOUT:
					DialogBox(EmuState.WindowInstance, (LPCTSTR)IDD_ABOUTBOX,
						hWnd, (DLGPROC)About);
				    break;
				case ID_AUDIO_CONFIG:
					OpenAudioConfig();
				    break;
				case ID_CPU_CONFIG:
					OpenCpuConfig();
				    break;
				case ID_DISPLAY_CONFIG:
					OpenDisplayConfig();
				    break;
				case ID_KEYBOARD_CONFIG:
					OpenInputConfig();
				    break;
				case ID_JOYSTICKS_CONFIG:
					OpenJoyStickConfig();
				    break;
				case ID_TAPE_CONFIG:
					OpenTapeConfig();
				    break;
				case ID_BITBANGER_CONFIG:
					OpenBitBangerConfig();
				    break;

				case ID_FILE_EXIT:
				case IDOK:
					SendMessage (hWnd, WM_CLOSE, 0, 0);
					break;

				case ID_FILE_RUN:
					EmuState.EmulationRunning=TRUE;
					InvalidateBoarder();
					break;

				case ID_CONTROL_HARD_RESET:
					if (EmuState.EmulationRunning)
						EmuState.ResetPending=2;
					break;

				case ID_CONTROL_SOFT_RESET:
					if (EmuState.EmulationRunning)
						EmuState.ResetPending=1;
					break;

				case ID_CONTROL_TOGGLE_DISPLAY_TYPE:
					SetMonitorType(!SetMonitorType(QUERY));
					break;

				case ID_CONTROL_TOGGLE_ARTIFACT_COLORS:
					FlipArtifacts();
					break;

				case ID_CONTROL_TOGGLE_THROTTLE:
					SetSpeedThrottle(!SetSpeedThrottle(QUERY));
					break;

				case ID_CONTROL_TOGGLE_OVERCLOCK:
					SetOverclock(!EmuState.OverclockFlag);
					SetupClock();
					break;

				case ID_CONTROL_OVERCLOCK_INCREASE:
					IncreaseOverclockSpeed();
					break;

				case ID_CONTROL_OVERCLOCK_DECREASE:
					DecreaseOverclockSpeed();
					break;

				case ID_CONTROL_TOGGLE_PAUSE:
					EmuState.Debugger.ToggleRun();
					break;

				case ID_CONTROL_SWAP_JOYSTICKS:
					SwapJoySticks();
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

				case ID_MEMORY_DISPLAY:
					VCC::Debugger::UI::OpenMemoryMapWindow(EmuState.WindowInstance, EmuState.WindowHandle);
				    break;
	
				case ID_PROCESSOR_STATE:
					VCC::Debugger::UI::OpenProcessorStateWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				case ID_BREAKPOINTS:
					VCC::Debugger::UI::OpenSourceDebugWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				case ID_MMU_MONITOR:
					VCC::Debugger::UI::OpenMMUMonitorWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				case ID_EXEC_TRACE:
					VCC::Debugger::UI::OpenExecutionTraceWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				case ID_DISASSEMBLER:
					VCC::OpenDisassemblerWindow(EmuState.WindowInstance, EmuState.WindowHandle);
					break;

				default:
				   return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_SETCURSOR:
			// Hide mouse cursor
            if ((EmuState.MousePointer != 1) && (LOWORD(lParam) == HTCLIENT)) {
				SetCursor(nullptr);
				return TRUE;
			}
			break;

		case WM_KILLFOCUS:
			// Force keys up if main widow keyboard focus is lost.  Otherwise
			// down keys will cause issues with OS-9 on return
			raise_saved_keys();
			break;

		case WM_CLOSE:
			CloseApp();
			break;

		case WM_CHAR:
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
	        OEMscan = (unsigned char) ((lParam >> 16) & 0xFF);
            Extended=(lParam >> 24) & 1;
		    if (Extended && (OEMscan!=DIK_NUMLOCK)) OEMscan += 0x80;
			vccKeyboardHandleKey(OEMscan,kEventKeyUp);
			
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
					if ( EmuState.EmulationRunning ) {
						EmuState.ResetPending = (IsShiftKeyDown()) ? 2 : 1;
					}
				break;

				case DIK_F6:
					if (IsShiftKeyDown())
						FlipArtifacts();
					else
						SetMonitorType(!SetMonitorType(QUERY));
				break;

				case DIK_F7:
					if (IsShiftKeyDown())
						SwapJoySticks();
					else
						EmuState.Debugger.ToggleRun();
				break;

				case DIK_F8:
					if (IsShiftKeyDown()) {
						SetOverclock(!EmuState.OverclockFlag);
						SetupClock();
					} else {
						SetSpeedThrottle(!SetSpeedThrottle(QUERY));
					}
				break;

				case DIK_F9:
					EmuState.EmulationRunning=!EmuState.EmulationRunning;
					if ( EmuState.EmulationRunning )
						EmuState.ResetPending=2;
					else
						SetStatusBarText("",&EmuState);
				break;

				case DIK_F10:
					if (EmuState.FullScreen)
					{
						if (const HMENU hMenu(IsShiftKeyDown()
											  ? GetConfMenu()
											  : GetSubMenu(GetMenu(hWnd), menu_bar_indexes::control_menu));
							hMenu != nullptr)
						{
							RECT r;
							GetClientRect(hWnd, &r);
							TrackPopupMenu(
								hMenu,
								TPM_CENTERALIGN | TPM_VCENTERALIGN,
								r.right / 2,
								r.bottom / 2,
								0,
								hWnd,
								nullptr);
						}
					}
				break;
				
				case DIK_F11:
					if (FlagEmuStop == TH_RUNNING) {
						if (IsShiftKeyDown()) {
							SetInfoBand(!SetInfoBand(QUERY));
							InvalidateBoarder();
						} else {
							FlagEmuStop = TH_REQWAIT;
							EmuState.FullScreen =!EmuState.FullScreen;
						}
					}
				break;

				case DIK_F12:
					if (IsShiftKeyDown())
					{
						DumpScreenshot();
					}
					
				break;

				default:
					// send shift and other keystrokes to the emulator if it is active
					if ( EmuState.EmulationRunning )
					{
						vccKeyboardHandleKey(OEMscan, kEventKeyDown);
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

					mouseXPosition = std::min(
						std::max(0, mouseXPosition - displayDetails.leftBorderColumns),
						maxHorizontalPosition);
					mouseYPosition = std::min(
						std::max(0, mouseYPosition - displayDetails.topBorderRows),
						maxVerticalPosition);

#if USE_DEBUG_MOUSE
					// render mouse content area
					DebugDrawBox(displayDetails.leftBorderColumns, displayDetails.topBorderRows, maxHorizontalPosition, maxVerticalPosition, VCC::ColorRed);

					// render mouse crosshair
					int mx = displayDetails.leftBorderColumns;
					int my = displayDetails.topBorderRows;
					DebugDrawLine(mouseXPosition + mx, mouseYPosition - 20 + my, mouseXPosition + mx, mouseYPosition + 20 + my, VCC::ColorRed);
					DebugDrawLine(mouseXPosition - 20 + mx, mouseYPosition + my, mouseXPosition + 20 + mx, mouseYPosition + my, VCC::ColorRed);
#endif
				}

				// Convert coordinates to mouseXPosition,mouseYPosition values with range 0-3fff (0-16383).
				mouseXPosition = static_cast<int>(mouseXPosition * (MAX_AXIS_VALUE / maxHorizontalPosition));
				mouseYPosition = static_cast<int>(mouseYPosition * (MAX_AXIS_VALUE / maxVerticalPosition));

				joystick(mouseXPosition, mouseYPosition);
			}

			return 0;
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
	if (SC_save1) vccKeyboardHandleKey(SC_save1,kEventKeyUp);
	if (SC_save2) vccKeyboardHandleKey(SC_save2,kEventKeyUp);
	SC_save1 = 0;
	SC_save2 = 0;
	return;
}

void SetCPUMultiplyerFlag (unsigned char double_speed)
{
	EmuState.DoubleSpeedFlag=double_speed;
	SetupClock();
	return;
}

void SetTurboMode(unsigned char data)
{
	EmuState.TurboSpeedFlag=(data&1)+1;
	SetupClock();
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

void SetupClock()
{
	int mult = (EmuState.OverclockFlag) ? EmuState.DoubleSpeedMultiplyer : 2;
	SetClockSpeed(1);
	if (EmuState.DoubleSpeedFlag)
		SetClockSpeed( mult * EmuState.TurboSpeedFlag);
	EmuState.CPUCurrentSpeed = .894;
	if (EmuState.DoubleSpeedFlag)
		EmuState.CPUCurrentSpeed*=(mult*EmuState.TurboSpeedFlag);
	return;
}

void DoHardReset(SystemState* const HRState)
{	
	HRState->RamBuffer=MmuInit(HRState->RamSize);	//Alocate RAM/ROM & copy ROM Images from source
	HRState->WRamBuffer=(unsigned short *)HRState->RamBuffer;

	// Debugger - watch memory.
	// Only Processor Space (64K) is watched at the moment.
	HRState->Debugger.Reset();
	// Debugger ---------------------

	// Remove all haltpoints
	VCC::KillHaltpoints();

	if (HRState->RamBuffer == nullptr)
	{
		MessageBox(nullptr,"Can't allocate enough RAM, Out of memory","Error",0);
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
	SetCPUMultiplyerFlag(0);
	SetClockSpeed(1);
	return;
}

void SoftReset()
{
	mc6883_reset();
	PiaReset();
	CPUReset();
	GimeReset();
	MmuReset();
	LoadRom();
	ResetBus();
	SetCPUMultiplyerFlag(0);
	EmuState.TurboSpeedFlag=1;
	return;
}

// Message handler for the About box.
BOOL CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDlg,IDC_TITLE,WM_SETTEXT,0,(LPARAM)(LPCSTR)g_szAppName);
			CenterDialog(hDlg);
			return TRUE;
		case WM_CLOSE:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		case WM_COMMAND:
			switch (LOWORD(wParam))
			case IDOK:
				SendMessage (hDlg, WM_CLOSE, 0, 0);
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
	if (throttle != QUERY)
	{
		EmuState.Throttle = throttle;
	}
	return(EmuState.Throttle);
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

void DoReboot()
{
	EmuState.ResetPending=2;
	return;
}

unsigned char SetAutoStart(unsigned char Tmp)
{
	if (Tmp != QUERY)
		AutoStart=Tmp;
	return AutoStart;
}

// LoadIniFile allows user to browse for an ini file and reloads the config from it.
void LoadIniFile()
{
	FileDialog dlg;
	dlg.setFilter("INI\0*.ini\0\0");
	dlg.setDefExt("ini");
	dlg.setInitialDir(AppDirectory() );
	dlg.setTitle(TEXT("Load Vcc Config File") );
	dlg.setFlags(OFN_FILEMUSTEXIST);

	// Send current ini file path to dialog
	char curini[MAX_PATH]="";
	GetIniFilePath(curini);
	dlg.setpath(curini);

	if ( dlg.show() ) {
		WriteIniFile();             // Flush current profile
		SetIniFilePath(dlg.path());   // Set new ini file path
		ReadIniFile();              // Load it
		UpdateConfig();
		EmuState.ResetPending = 2;
	}
	return;
}

// SaveConfig copies the current ini file to a choosen ini file.
void SaveConfig() {

	FileDialog dlg;
	dlg.setFilter("INI\0*.ini\0\0");
	dlg.setDefExt("ini");
	dlg.setInitialDir(AppDirectory() );
	dlg.setTitle(TEXT("Save Vcc Config File") );
	dlg.setFlags(OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT);

	// Send current ini file path to dialog
	char curini[MAX_PATH]="";
	GetIniFilePath(curini);
	dlg.setpath(curini);

	if ( dlg.show(1) ) {
		SetIniFilePath(dlg.path());   // Set new ini file path
		WriteIniFile();             // Flush current profile
		// If ini file has changed
		if (_stricmp(curini,dlg.path()) != 0) {
			// Copy current ini to new ini
			if (! CopyFile(curini,dlg.path(),false) )
				MessageBox(nullptr,"Copy config failed","error",0);
		}
	}
	return;
}

void CALLBACK update_status(HWND, UINT, UINT_PTR, DWORD)
{
	GetModuleStatus(&EmuState);

	char tstatus[128];
	char tspeed[32];
	snprintf(tspeed,sizeof(tspeed),"%2.2fMhz",EmuState.CPUCurrentSpeed);
	// Append "+" to speed if overclocking is enabled
	if (EmuState.OverclockFlag && (EmuState.DoubleSpeedMultiplyer>2))
		strncat (tspeed,"+",sizeof(tspeed));
	if (EmuState.Debugger.IsHalted()) {
		snprintf(tstatus,sizeof(tstatus), " Paused - Hit F7 | %s @ %s | %s",
				 CpuName,tspeed,EmuState.StatusLine);
	} else {
		snprintf(tstatus,sizeof(tstatus),"Skip:%2.2i | FPS:%3.0f | %s @ %s | %s",
				 EmuState.FrameSkip,EmuState.FPS,CpuName,tspeed,EmuState.StatusLine);
	}
	int len = strlen(tstatus);
	UpdateTapeStatus(tstatus + len, sizeof(tstatus) - len);
	SetStatusBarText(tstatus,&EmuState);
}

unsigned __stdcall EmuLoop(HANDLE hEvent)
{
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

		if ((Qflag==255) && (FrameCounter==32))
		{
			Qflag=0;
			QuickLoad(CmdArg.QLoadFile);
		}

		if ((Pflag==255) && (FrameCounter==32)) {
			Pflag=0;
			QueueText(CmdArg.PasteText);
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
			if (EmuState.EmulationRunning == 1) {
				FPS += RenderFrame(&EmuState);
			} else {
				FPS += Static(&EmuState);
			}
		}
		EndRender(EmuState.FrameSkip);
		FPS/=EmuState.FrameSkip;
		EmuState.FPS = FPS;

		if (CmdArg.MaxFrames > 0 && FrameCounter >= CmdArg.MaxFrames)
		{
			if (*CmdArg.Screenshot != 0)
				DumpScreenshot(CmdArg.Screenshot);
			CloseApp();
		}

		if (EmuState.Throttle)	//Do nothing untill the frame is over returning unused time to OS
			FrameWait();
	} //Still Emulating

	return 0;
}



void FullScreenToggle()
{
	PauseAudio(true);
	if (!CreateDDWindow(&EmuState))
	{
		MessageBox(nullptr,"Can't rebuild primary Window","Error",0);
		exit(0);
	}
	InvalidateBoarder();

	EmuState.ConfigDialog=nullptr;
	PauseAudio(false);
	return;
}


HMENU GetConfMenu()
{
	static HMENU hMenu = nullptr;
	if (hMenu == nullptr) {
		hMenu = CreatePopupMenu();
		AppendMenu(hMenu,MF_STRING,ID_AUDIO_CONFIG,"Audio");
		AppendMenu(hMenu,MF_STRING,ID_CPU_CONFIG,"Cpu");
		AppendMenu(hMenu,MF_STRING,ID_DISPLAY_CONFIG,"Display");
		AppendMenu(hMenu,MF_STRING,ID_KEYBOARD_CONFIG,"Keyboard");
		AppendMenu(hMenu,MF_STRING,ID_JOYSTICKS_CONFIG,"Joysticks");
		AppendMenu(hMenu,MF_STRING,ID_TAPE_CONFIG,"Tape");
		AppendMenu(hMenu,MF_STRING,ID_BITBANGER_CONFIG,"BitBanger");
	}
	return hMenu;
}

bool IsShiftKeyDown()
{
  return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
}



static void InsertRomPakCartridge()
{
	SetThreadDescription(GetCurrentThread(), L"** ROM Pak Cartridge Loader UI Thread");

	const auto insert_rom_cartridge = [](const auto& filename)
	{
		const auto result(PakInsertRom(filename));

		if (result == vcc::utils::cartridge_loader_status::success)
		{
			SetStartupCartridge(filename);
			PakSetLastAccessedRomPakPath(filename.parent_path());
		}

		return result;
	};

	::vcc::utils::select_rompak_cartridge_file(
		GetActiveWindow(),
		"Insert ROM Pak Cartridge",
		PakGetLastAccessedRomPakPath(),
		insert_rom_cartridge);
}

static void ProcessSelectDeviceCartridgeMenuItem(const vcc::utils::cartridge_catalog::item_type& catalog_item)
{
	if (const auto result(PakInsertCartridge(catalog_item.id));
		result != ::vcc::utils::cartridge_loader_status::success)
	{
		auto error_string(::vcc::utils::load_error_string(result) + "\n\n" + catalog_item.name);

		MessageBox(
			EmuState.WindowHandle,
			error_string.c_str(),
			"Load Error",
			MB_OK | MB_ICONERROR);

		return;
	}

	SetStartupCartridge(catalog_item.id);
}

static void LoadStartupCartridge()
{
	static const auto default_error_message(
		"Unable to insert cartridge. The cartridge location stored in the settings is in an invalid or unknown format.\n"
		"\n"
		"The cartridge location will be reset.");
	static const auto error_title("Startup Error - Unable To Insert Cartridge");


	auto location_text(GetCartridgeLocation());
	if (location_text.empty())
	{
		return;
	}

	std::string error_message;

	if (const auto optional_location(::vcc::utils::resource_location::from_string(location_text));
		optional_location.has_value())
	{
		const auto& location(optional_location.value());

		std::optional<::vcc::utils::cartridge_loader_status> insert_result;
		if (location.is_path())
		{
			insert_result = PakInsertRom(location.path());
		}
		else if (location.is_guid())
		{
			insert_result = PakInsertCartridge(location.guid());
		}

		if (insert_result.has_value())
		{
			if (*insert_result == ::vcc::utils::cartridge_loader_status::success)
			{
				return;
			}

			error_message = ::vcc::utils::load_error_string(*insert_result);
		}
	}

	MessageBox(
		EmuState.WindowHandle,
		error_message.empty() ? default_error_message : error_message.c_str(),
		error_title,
		MB_OK | MB_ICONERROR);
	ClearStartupCartridge();
}
