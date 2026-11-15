//#define USE_LOGGING
//======================================================================
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
//======================================================================

/********************************************************************************************
*	fd502.cpp : Defines the entry point for the DLL application.							*
*	DLL for use with Vcc 1.0 or higher. DLL interface 1.0 Beta								*
*	This Module will emulate a Tandy Floppy Disk Model FD-502 With 3 DSDD drives attached	*
*	Copyright 2006 (c) by Joseph Forgione 													*
*********************************************************************************************/

#pragma warning( disable : 4800 ) // For legacy builds

#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <vector>
#include <filesystem>
#include "resource.h"
#include "wd1793.h"
#include "distortc.h"
#include "fd502.h"
#include <vcc/util/limits.h>
#include <vcc/util/FileOps.h>
#include <vcc/util/DialogOps.h>
#include <vcc/util/logger.h>
// Three includes added for PakGetMenuItem
#include <vcc/bus/cartridge_menu.h>
#include <vcc/bus/cartridge_messages.h>
#include <vcc/util/fileutil.h>

//include becker code if COMBINE_BECKER is defined in fd502.h
#ifdef COMBINE_BECKER
#include "becker.h"
#endif

// Settings module groups used
const std::string MS_FD502  = "FD-502";
const std::string MS_BECKER = "DW Becker";
const std::string MS_PATHS  = "DefaultPaths";

constexpr auto EXTROMSIZE = 16384u;

using namespace std;

extern DiskInfo gVirtualDrive[6];
static unsigned char ExternalRom[EXTROMSIZE];
static unsigned char DiskRom[EXTROMSIZE];
static unsigned char RGBDiskRom[EXTROMSIZE];
static char FloppyPath[MAX_PATH];
static char RomFileName[MAX_PATH]="";
static char TempRomFileName[MAX_PATH]="";
slot_id_type gSlotId {};
PakAssertInteruptHostCallback AssertInt = nullptr;
unsigned char PhysicalDriveA=0,PhysicalDriveB=0,OldPhysicalDriveA=0,OldPhysicalDriveB=0;
static unsigned char *RomPointer[3]={ExternalRom,DiskRom,RGBDiskRom};
static unsigned char SelectRom=0;
unsigned char SetChip(unsigned char);
static unsigned char NewDiskNumber=0,CreateFlag=0;
static unsigned char PersistDisks=0;
static unsigned char TempSelectRom=0;
static unsigned char ClockEnabled=1;
LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NewDisk(HWND,UINT, WPARAM, LPARAM);
void LoadConfig();
void SaveConfig();
long CreateDiskHeader(const char *,unsigned char,unsigned char,unsigned char);
void Unload_Disk(unsigned char);
void Load_Disk(unsigned char);

static HWND g_hConfDlg = nullptr;
static HINSTANCE gModuleInstance;
static unsigned long RealDisks=0;
long CreateDisk (unsigned char);
static char gSelectedDiskFile[MAX_PATH]="";
unsigned char LoadExtRom( unsigned char, const char *);

// Added for PakGetMenuItem export
bool get_menu_item(menu_item_entry*, size_t index);

int BeckerEnabled=0;
char BeckerAddr[MAX_PATH]="";
char BeckerPort[32]="";

// Added for PakGetMenuItem export
static HWND gVccWnd = nullptr;
static VCC::Util::settings* gpSettings = nullptr; // Pointer to settings object
static bool gPakInitialized = false;
static PakSetCpuMemoryWriteWatchHostCallback SetCpuMemoryWriteWatch = nullptr;

struct AutoSequenceRequest
{
	unsigned char value = 0;
	unsigned short address = 0;
};

static std::mutex gAutoSequenceMutex;
static std::vector<std::string> gAutoSequencePlaylist;
static std::deque<AutoSequenceRequest> gAutoSequenceRequests;
static unsigned short gAutoSequenceAddress = 0x7F00;
static constexpr size_t AUTO_SEQUENCE_MAX_PENDING_REQUESTS = 16;
static constexpr unsigned char AUTO_SEQUENCE_STAGING_DRIVE = 5;
static bool gAutoSequenceWrap = true;
static int gAutoSequenceCurrentEntry = -1;
static const char* const AUTO_SEQUENCE_SECTION = "FD-502 AutoSequence";

struct AutoSequenceDialogState
{
	std::vector<std::string> playlist;
	unsigned short address = 0x7F00;
	bool wrap = true;
	std::string current_path;
};

static INT_PTR CALLBACK AutoSequenceDialogProc(HWND, UINT, WPARAM, LPARAM);
static void OpenAutoSequenceDialog(HWND owner);
static void LoadAutoSequenceConfig();
static void SaveAutoSequenceConfigLocked();
static void RegisterAutoSequenceWatch();
static void ProcessPendingAutoSequence();
static bool ReplaceDrive3Image(const std::string& path);


//----------------------------------------------------------------------------
extern "C"
{

	__declspec(dllexport) const char* PakGetName()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_MODULE_NAME, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetCatalogId()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_CATNUMBER, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) const char* PakGetDescription()
	{
		static char string_buffer[MAX_LOADSTRING];

		LoadString(gModuleInstance, IDS_DESCRIPTION, string_buffer, MAX_LOADSTRING);

		return string_buffer;
	}

	__declspec(dllexport) void PakInitialize(
		slot_id_type SlotId,
		const char* const configuration_path,
		HWND hVccWnd,
		const cpak_callbacks* const callbacks)
	{
		if (!callbacks || !configuration_path) return;
		gSlotId = SlotId;
		gVccWnd = hVccWnd;
		AssertInt = callbacks->assert_interrupt;
		SetCpuMemoryWriteWatch = callbacks->set_cpu_memory_write_watch;
		if (gpSettings) {
			delete gpSettings;
			gpSettings = nullptr;
		}
		gpSettings = new VCC::Util::settings(configuration_path);
		RealDisks = InitController();
		LoadConfig();
		LoadAutoSequenceConfig();
		RegisterAutoSequenceWatch();
		gPakInitialized = true;
		SendMessage(gVccWnd,WM_VCC_UPD_MENU,(WPARAM) 0,(LPARAM) 0);
	}

	__declspec(dllexport) void PakReset()
	{
		ResetController();
		LoadExtRom(External,RomFileName);
	}

	__declspec(dllexport) void PakSetWindowHandle(HWND hVccWnd)
	{
		gVccWnd = hVccWnd;
	}

	// Every CPU store is queued independently, even when consecutive values
	// are identical. The MMU observer never consumes the real RAM write.
	__declspec(dllexport) void PakCpuMemoryWrite(unsigned char value, unsigned short address)
	{
		std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
		if (gAutoSequencePlaylist.empty() || gAutoSequenceAddress >= 0xFFFF ||
			(address != gAutoSequenceAddress &&
			 address != static_cast<unsigned short>(gAutoSequenceAddress + 1)))
			return;
		if (gAutoSequenceRequests.size() >= AUTO_SEQUENCE_MAX_PENDING_REQUESTS) {
			DLOG_C("AutoSequence: command queue full; dropping command\\n");
			return;
		}
		gAutoSequenceRequests.push_back({ value, address });
	}

	__declspec(dllexport) bool PakCpuMemoryRead(unsigned short address, unsigned char* value)
	{
		if (!value) return false;
		std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
		if (gAutoSequencePlaylist.empty() || gAutoSequenceAddress >= 0xFFFF ||
			address != static_cast<unsigned short>(gAutoSequenceAddress + 1))
			return false;
		*value = (gAutoSequenceCurrentEntry >= 0 &&
			gAutoSequenceCurrentEntry < static_cast<int>(gAutoSequencePlaylist.size()))
			? static_cast<unsigned char>(gAutoSequenceCurrentEntry + 1) : 0;
		return true;
	}

	__declspec(dllexport) void PakTerminate()
	{
		if (!gPakInitialized && gpSettings == nullptr)
			return;
		if (SetCpuMemoryWriteWatch)
			SetCpuMemoryWriteWatch(gSlotId, gAutoSequenceAddress, false);
		SetCpuMemoryWriteWatch = nullptr;
		{
			std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
			gAutoSequenceRequests.clear();
		}
		gPakInitialized = false;
#ifdef COMBINE_BECKER
		becker_enable(0);
#endif
		CloseCartDialog(g_hConfDlg);
		for (unsigned char dnum = 0; dnum <= 3; dnum++)
			unmount_disk_image(dnum);
		unmount_disk_image(AUTO_SEQUENCE_STAGING_DRIVE);
		delete gpSettings;
		gpSettings = nullptr;
	}

	//PakGetMenuItem export
	__declspec(dllexport) bool PakGetMenuItem(menu_item_entry* item, size_t index)
	{
		return get_menu_item(item, index);
	}

	__declspec(dllexport) void PakMenuItemClicked(unsigned char MenuID)
	{
		HWND h_own = GetActiveWindow();
		switch (MenuID)
		{
			case 10:
				Load_Disk(0);
				break;
			case 11:
				Unload_Disk(0);
				break;
			case 12:
				Load_Disk(1);
				break;
			case 13:
				Unload_Disk(1);
				break;
			case 14:
				Load_Disk(2);
				break;
			case 15:
				Unload_Disk(2);
				break;
			case 17:
				Load_Disk(3);
				break;
			case 18:
				Unload_Disk(3);
				break;
			case 19:
				OpenAutoSequenceDialog(h_own);
				break;
			case 16:
				if (g_hConfDlg == nullptr)
					g_hConfDlg = CreateDialog(
							gModuleInstance,(LPCTSTR)IDD_CONFIG,h_own,(DLGPROC)Config);
				ShowWindow(g_hConfDlg,1);
				break;
		}
		return;
	}

	__declspec(dllexport) void PakWritePort(unsigned char Port,unsigned char Data)
	{
#ifdef COMBINE_BECKER
		if (BeckerEnabled && (Port == 0x42)) {
			becker_write(Data,Port);
		} else //if ( ((Port == 0x50) | (Port==0x51)) & ClockEnabled) {
#endif
		if ( ((Port == 0x50) | (Port==0x51)) & ClockEnabled) {
			write_time(Data,Port);
		} else {
			disk_io_write(Data,Port);
		}
		return;
	}

	__declspec(dllexport) unsigned char PakReadPort(unsigned char Port)
	{
#ifdef COMBINE_BECKER
		if (BeckerEnabled && ((Port == 0x41) | (Port== 0x42 )))
			return(becker_read(Port));
#endif
		if ( ((Port == 0x50) | (Port== 0x51 )) & ClockEnabled)
			return(read_time(Port));
		return(disk_io_read(Port));
	}

	__declspec(dllexport) void PakProcessHorizontalSync()
	{
		PingFdc();
		ProcessPendingAutoSequence();
		return;
	}

	__declspec(dllexport) unsigned char PakReadMemoryByte(unsigned short Address)
	{
		return(RomPointer[SelectRom][Address & (EXTROMSIZE-1)]);
	}

	__declspec(dllexport) void PakGetStatus(char* text_buffer, size_t buffer_size)
	{
		char diskstat[64];
		DiskStatus(diskstat, sizeof(diskstat));
		strcpy(text_buffer,diskstat);
#ifdef COMBINE_BECKER
		char beckerstat[64];
		if(BeckerEnabled) {
			becker_status(beckerstat);
			strcat(text_buffer," | ");
			strcat(text_buffer,beckerstat);
		}
#endif
		return ;
	}
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD reason,        // reason for calling function
    LPVOID lpReserved )  // reserved
{
	if (reason == DLL_PROCESS_ATTACH) {
		gModuleInstance = hinstDLL;
	} else if (reason == DLL_PROCESS_DETACH) {
		PakTerminate();
	}
	return TRUE;
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	static unsigned char temp=0,temp2=0;
	long ChipChoice[3]={IDC_EXTROM,IDC_TRSDOS,IDC_RGB};
	long gVirtualDrive[2]={IDC_DISKA,IDC_DISKB};
	char VirtualNames[5][16]={"None","Drive 0","Drive 1","Drive 2","Drive 3"};

	switch (message)
	{
		case WM_CLOSE:
			DestroyWindow(hDlg);
			g_hConfDlg = nullptr;
			return TRUE;

		case WM_INITDIALOG:
			CenterDialog(hDlg);
			TempSelectRom=SelectRom;
			if (!RealDisks)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_DISKA), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_DISKB), FALSE);
				PhysicalDriveA=0;
				PhysicalDriveB=0;
			}
			SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
			OldPhysicalDriveA=PhysicalDriveA;
			OldPhysicalDriveB=PhysicalDriveB;
			strcpy(TempRomFileName,RomFileName);
			SendDlgItemMessage(hDlg,IDC_TURBO,BM_SETCHECK,SetTurboDisk(QUERY),0);
			SendDlgItemMessage(hDlg,IDC_PERSIST,BM_SETCHECK,PersistDisks,0);
			for (temp=0;temp<sizeof(ChipChoice) / sizeof(*ChipChoice);temp++)
				SendDlgItemMessage(hDlg,ChipChoice[temp],BM_SETCHECK,(temp==TempSelectRom),0);
			for (temp=0;temp<2;temp++)
				for (temp2=0;temp2<5;temp2++)
						SendDlgItemMessage (hDlg,gVirtualDrive[temp], CB_ADDSTRING, 0, (LPARAM) VirtualNames[temp2]);

			SendDlgItemMessage (hDlg, IDC_DISKA,CB_SETCURSEL,(WPARAM)PhysicalDriveA,(LPARAM)0);
			SendDlgItemMessage (hDlg, IDC_DISKB,CB_SETCURSEL,(WPARAM)PhysicalDriveB,(LPARAM)0);
			SendDlgItemMessage (hDlg,IDC_ROMPATH,WM_SETTEXT,0,(LPARAM)(LPCSTR)TempRomFileName);

			SendDlgItemMessage (hDlg,IDC_BECKER_ENAB,BM_SETCHECK,BeckerEnabled,0);
			SendDlgItemMessage (hDlg,IDC_BECKER_HOST,WM_SETTEXT,0,(LPARAM)(LPSTR)BeckerAddr);
			SendDlgItemMessage (hDlg,IDC_BECKER_PORT,WM_SETTEXT,0,(LPARAM)(LPCSTR)BeckerPort);

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					ClockEnabled=(unsigned char)SendDlgItemMessage(hDlg,IDC_CLOCK,BM_GETCHECK,0,0);
					SetTurboDisk((unsigned char)SendDlgItemMessage(hDlg,IDC_TURBO,BM_GETCHECK,0,0));
					PersistDisks=(unsigned char) SendDlgItemMessage(hDlg,IDC_PERSIST,BM_GETCHECK,0,0);
					PhysicalDriveA=(unsigned char)SendDlgItemMessage(hDlg,IDC_DISKA,CB_GETCURSEL,0,0);
					PhysicalDriveB=(unsigned char)SendDlgItemMessage(hDlg,IDC_DISKB,CB_GETCURSEL,0,0);
					if (!RealDisks)
					{
						PhysicalDriveA=0;
						PhysicalDriveB=0;
					//	MessageBox(0,"Wrong Version or Driver not installed","FDRAWCMD Driver",0);
					}
					else
					{
						if (PhysicalDriveA != OldPhysicalDriveA)	//Drive changed
						{
							if (OldPhysicalDriveA!=0)
								unmount_disk_image(OldPhysicalDriveA-1);
							if (PhysicalDriveA!=0)
								mount_disk_image("*Floppy A:",PhysicalDriveA-1);
						}
						if (PhysicalDriveB != OldPhysicalDriveB)	//Drive changed
						{
							if (OldPhysicalDriveB!=0)
								unmount_disk_image(OldPhysicalDriveB-1);
							if (PhysicalDriveB!=0)
								mount_disk_image("*Floppy B:",PhysicalDriveB-1);
						}
					}
					SelectRom=TempSelectRom;
					strcpy(RomFileName,TempRomFileName );
					CheckPath(RomFileName);
					LoadExtRom(External,RomFileName); //JF
#ifdef COMBINE_BECKER
					GetDlgItemText(hDlg,IDC_BECKER_HOST,BeckerAddr,MAX_PATH);
					GetDlgItemText(hDlg,IDC_BECKER_PORT,BeckerPort,32);
					becker_sethost(BeckerAddr,BeckerPort);
					BeckerEnabled = SendDlgItemMessage (hDlg,IDC_BECKER_ENAB,BM_GETCHECK,0,0);
					becker_enable(BeckerEnabled);
#endif
					SaveConfig();
					DestroyWindow(hDlg);
					g_hConfDlg = nullptr;
					return TRUE;

				case IDC_EXTROM:
				case IDC_TRSDOS:
				case IDC_RGB:
					for (temp=0;temp<sizeof(ChipChoice) / sizeof(*ChipChoice);temp++)
						if (LOWORD(wParam)==ChipChoice[temp])
						{
							for (temp2=0;temp2<sizeof(ChipChoice) / sizeof(*ChipChoice);temp2++)
								SendDlgItemMessage(hDlg,ChipChoice[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,ChipChoice[temp],BM_SETCHECK,1,0);
							TempSelectRom=temp;
						}
					break;
				case IDC_BROWSE:
					{
						char dir[MAX_PATH];
						strncpy(dir,TempRomFileName,MAX_PATH);
						if (char * p = strrchr(dir,'\\')) *p = '\0';

						FileDialog dlg;
						dlg.setFilter("Disk Rom Images\0*.rom;\0All files\0*.*\0\0");
						dlg.setTitle("Select Disk Rom Image");
						dlg.setInitialDir(dir);
						if (dlg.show(0,g_hConfDlg)) {
							dlg.getpath(TempRomFileName,MAX_PATH);
							SendDlgItemMessage
								(hDlg,IDC_ROMPATH,WM_SETTEXT,0,(LPARAM)dlg.path());
						}
					}
					break;
				case IDC_CLOCK:
				case IDC_READONLY:
					break;

				case IDCANCEL:
					DestroyWindow(hDlg);
					g_hConfDlg = nullptr;
					break;
			}
			return TRUE;

	}
    return FALSE;
}

// Access the settings object
VCC::Util::settings& Setting()
{
	return *gpSettings;
}

void Unload_Disk(unsigned char disk) {
	unmount_disk_image(disk);
	SendMessage(gVccWnd,WM_VCC_UPD_MENU,(WPARAM) 0,(LPARAM) 0);
	SaveConfig();
}

void Load_Disk(unsigned char disk)
{
	HWND h_own = GetActiveWindow();
	FileDialog dlg;
	dlg.setInitialDir(FloppyPath);
	dlg.setFilter("Disk Images\0*.dsk;*.os9\0\0");
	dlg.setDefExt("dsk");
	dlg.setTitle("Insert Disk Image");
	dlg.setFlags(OFN_PATHMUSTEXIST);
	if (dlg.show(0,h_own)) {
		CreateFlag = 1;
		HANDLE hr = nullptr;
		dlg.getpath(gSelectedDiskFile,MAX_PATH);
		// Verify file exists
		hr = CreateFile
			(gSelectedDiskFile,0,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
		// Create new disk file if it does not
		if (hr == INVALID_HANDLE_VALUE) {
			NewDiskNumber = disk;
			//DialogBox will set CreateFlag = 0 on cancel
			DialogBox(gModuleInstance, (LPCTSTR)IDD_NEWDISK, h_own, (DLGPROC)NewDisk);
		} else {
			CloseHandle(hr);
		}
		// Attempt mount if file existed or file create was not canceled
		if (CreateFlag==1) {
			if (mount_disk_image(gSelectedDiskFile,disk)==0) {
				MessageBox(g_hConfDlg,"Can't open file","Error",0);
			} else {
				dlg.getdir(FloppyPath);
				SendMessage(gVccWnd,WM_VCC_UPD_MENU,(WPARAM) 0,(LPARAM) 0);
				SaveConfig();
			}
		}
	}
	return;
}

unsigned char SetChip(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		SelectRom=Tmp;
	return SelectRom;
}

// Added for PakGetMenuItem export
// Return items for cartridge menu. Called for each item
bool get_menu_item(menu_item_entry* item, size_t index)
{
	using VCC::Bus::gDllCartMenu;
	std::string disk;
	if (!item) return false;
	// request zero rebuilds the menu list
	if (index == 0) {
		gDllCartMenu.clear();
		gDllCartMenu.add("", 0, MIT_Seperator);
		gDllCartMenu.add("FD-502 Drive 0",0,MIT_Head);
		gDllCartMenu.add("Insert",ControlId(10),MIT_Slave);
		disk = VCC::Util::GetFileNamePart(gVirtualDrive[0].ImageName);
		gDllCartMenu.add("Eject: "+disk,ControlId(11),MIT_Slave);
		gDllCartMenu.add("FD-502 Drive 1",0,MIT_Head);
		gDllCartMenu.add("Insert",ControlId(12),MIT_Slave);
		disk = VCC::Util::GetFileNamePart(gVirtualDrive[1].ImageName);
		gDllCartMenu.add("Eject: "+disk,ControlId(13),MIT_Slave);
		gDllCartMenu.add("FD-502 Drive 2",0,MIT_Head);
		gDllCartMenu.add("Insert",ControlId(14),MIT_Slave);
		disk = VCC::Util::GetFileNamePart(gVirtualDrive[2].ImageName);
		gDllCartMenu.add("Eject: "+disk,ControlId(15),MIT_Slave);
		gDllCartMenu.add("FD-502 Drive 3",0,MIT_Head);
		gDllCartMenu.add("Insert",ControlId(17),MIT_Slave);
		disk = VCC::Util::GetFileNamePart(gVirtualDrive[3].ImageName);
		gDllCartMenu.add("Eject: "+disk,ControlId(18),MIT_Slave);
		gDllCartMenu.add("FD-502 Drive 3 AutoSequence",0,MIT_Head);
		gDllCartMenu.add("Playlist / Settings",ControlId(19),MIT_Slave);
		gDllCartMenu.add("FD-502 Config",ControlId(16),MIT_StandAlone);
	}
	// return requested list item or false
	return gDllCartMenu.copy_item(*item, index);
}

long CreateDisk (unsigned char Disk)
{
	NewDiskNumber=Disk;
	HWND h_own = GetActiveWindow();
	DialogBox(gModuleInstance, (LPCTSTR)IDD_NEWDISK, h_own, (DLGPROC)NewDisk);
	return 0;
}

LRESULT CALLBACK NewDisk(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	unsigned char temp=0,temp2=0;
	static unsigned char NewDiskType=JVC,NewDiskTracks=2,DblSided=1;
	char Dummy[MAX_PATH]="";
	long DiskType[3]={IDC_JVC,IDC_VDK,IDC_DMK};
	long DiskTracks[3]={IDC_TR35,IDC_TR40,IDC_TR80};

	switch (message)
	{
		case WM_CLOSE:
			EndDialog(hDlg,LOWORD(wParam));  //Modal dialog
			return TRUE;

		case WM_INITDIALOG:
			for (temp=0;temp<=2;temp++)
				SendDlgItemMessage(hDlg,DiskType[temp],BM_SETCHECK,(temp==NewDiskType),0);
			for (temp=0;temp<=2;temp++)
				SendDlgItemMessage(hDlg,DiskTracks[temp],BM_SETCHECK,(temp==NewDiskTracks),0);
			SendDlgItemMessage(hDlg,IDC_DBLSIDE,BM_SETCHECK,DblSided,0);
			strcpy(Dummy,gSelectedDiskFile);
			PathStripPath(Dummy);
			SendDlgItemMessage(hDlg,IDC_TEXT1,WM_SETTEXT,0,(LPARAM)(LPCSTR)Dummy);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_DMK:
				case IDC_JVC:
				case IDC_VDK:
					for (temp=0;temp<=2;temp++)
						if (LOWORD(wParam)==DiskType[temp])
						{
							for (temp2=0;temp2<=2;temp2++)
								SendDlgItemMessage(hDlg,DiskType[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,DiskType[temp],BM_SETCHECK,1,0);
							NewDiskType=temp;
						}
				break;

				case IDC_TR35:
				case IDC_TR40:
				case IDC_TR80:
					for (temp=0;temp<=2;temp++)
						if (LOWORD(wParam)==DiskTracks[temp])
						{
							for (temp2=0;temp2<=2;temp2++)
								SendDlgItemMessage(hDlg,DiskTracks[temp2],BM_SETCHECK,0,0);
							SendDlgItemMessage(hDlg,DiskTracks[temp],BM_SETCHECK,1,0);
							NewDiskTracks=temp;
						}
				break;

				case IDC_DBLSIDE:
					DblSided=!DblSided;
					SendDlgItemMessage(hDlg,IDC_DBLSIDE,BM_SETCHECK,DblSided,0);
				break;

				case IDOK:
				{
					EndDialog(hDlg, LOWORD(wParam));
					const char *ext = gSelectedDiskFile + strlen(gSelectedDiskFile) - 4;
					if (ext < gSelectedDiskFile) ext = gSelectedDiskFile;
					// if it doesn't end with .dsk, append it
					if (_stricmp(ext, ".dsk") != 0)
					{
						PathRemoveExtension(gSelectedDiskFile);
						strcat(gSelectedDiskFile, ".dsk");
					}
					if (CreateDiskHeader(gSelectedDiskFile, NewDiskType, NewDiskTracks, DblSided))
					{
						strcpy(gSelectedDiskFile, "");
						MessageBox(g_hConfDlg, "Can't create File", "Error", 0);
					}
					return TRUE;
				}

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					CreateFlag=0;
					return FALSE;
			}
			return TRUE;
	}
    return FALSE;
}

long CreateDiskHeader(const char *FileName,unsigned char Type,unsigned char Tracks,unsigned char DblSided)
{
	HANDLE hr=nullptr;
	unsigned char Dummy=0;
	unsigned char HeaderBuffer[16]="";
	unsigned char TrackTable[3]={35,40,80};
	unsigned short TrackSize=0x1900;
	unsigned char IgnoreDensity=0,SingleDensity=0,HeaderSize=0;
	unsigned long BytesWritten=0,FileSize=0;
	hr=CreateFile( FileName,GENERIC_READ | GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
	if (hr==INVALID_HANDLE_VALUE)
		return 1; //Failed to create File

	switch (Type)
	{
		case DMK:
			HeaderBuffer[0]=0;
			HeaderBuffer[1]=TrackTable[Tracks];
			HeaderBuffer[2]=(TrackSize & 0xFF);
			HeaderBuffer[3]=(TrackSize >>8);
			HeaderBuffer[4]=(IgnoreDensity<<7) | (SingleDensity<<6) | ((!DblSided)<<4);
			HeaderBuffer[0xC]=0;
			HeaderBuffer[0xD]=0;
			HeaderBuffer[0xE]=0;
			HeaderBuffer[0xF]=0;
			HeaderSize=0x10;
			FileSize= HeaderSize + (HeaderBuffer[1] * TrackSize * (DblSided+1) );
		break;

		case JVC:	// has variable header size
			HeaderSize=0;
			HeaderBuffer[0]=18;			//18 Sectors
			HeaderBuffer[1]=DblSided+1;	// Double or single Sided;
			FileSize = (HeaderBuffer[0] * 0x100 * TrackTable[Tracks] * (DblSided+1));
			if (DblSided)
			{
				FileSize+=2;
				HeaderSize=2;
			}
		break;

		case VDK:
			HeaderBuffer[9]=DblSided+1;
			HeaderSize=12;
			FileSize = ( 18 * 0x100 * TrackTable[Tracks] * (DblSided+1));
			FileSize+=HeaderSize;
		break;

		case 3:
			HeaderBuffer[0]=0;
			HeaderBuffer[1]=TrackTable[Tracks];
			HeaderBuffer[2]=(TrackSize & 0xFF);
			HeaderBuffer[3]=(TrackSize >>8);
			HeaderBuffer[4]=((!DblSided)<<4);
			HeaderBuffer[0xC]=0x12;
			HeaderBuffer[0xD]=0x34;
			HeaderBuffer[0xE]=0x56;
			HeaderBuffer[0xF]=0x78;
			HeaderSize=0x10;
			FileSize=1;
		break;

	}
	SetFilePointer(hr,0,nullptr,FILE_BEGIN);
	WriteFile(hr,HeaderBuffer,HeaderSize,&BytesWritten,nullptr);
	SetFilePointer(hr,FileSize-1,nullptr,FILE_BEGIN);
	WriteFile(hr,&Dummy,1,&BytesWritten,nullptr);
	CloseHandle(hr);
	return 0;
}

static void LoadAutoSequenceConfig()
{
	std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
	gAutoSequencePlaylist.clear();
	gAutoSequenceRequests.clear();

	int address = Setting().read(AUTO_SEQUENCE_SECTION, "MemoryAddress", 0x7F00);
	// The command and status/readback bytes must not wrap past the address space.
	if (address < 0 || address > 0xFFFE) address = 0x7F00;
	gAutoSequenceAddress = static_cast<unsigned short>(address);
	gAutoSequenceWrap = Setting().read(AUTO_SEQUENCE_SECTION, "Wrap", 1) != 0;

	int count = Setting().read(AUTO_SEQUENCE_SECTION, "PlaylistCount", 0);
	if (count < 0) count = 0;
	if (count > 255) count = 255;

	for (int i = 1; i <= count; ++i)
	{
		char key[32] = {};
		sprintf(key, "Entry%03d", i);
		std::string disk_path = Setting().read(AUTO_SEQUENCE_SECTION, key, "");
		if (!disk_path.empty())
			gAutoSequencePlaylist.push_back(disk_path);
	}

	int current = Setting().read(AUTO_SEQUENCE_SECTION, "CurrentEntry", 0) - 1;
	gAutoSequenceCurrentEntry = (current >= 0 && current < static_cast<int>(gAutoSequencePlaylist.size()))
		? current : -1;

	// When the persisted Drive 3 image is part of the playlist, use that as
	// the authoritative current entry. Otherwise retain the saved playlist index.
	if (gVirtualDrive[3].ImageName[0])
	{
		for (size_t i = 0; i < gAutoSequencePlaylist.size(); ++i)
		{
			if (_stricmp(gVirtualDrive[3].ImageName, gAutoSequencePlaylist[i].c_str()) == 0)
			{
				gAutoSequenceCurrentEntry = static_cast<int>(i);
				break;
			}
		}
	}
}

static void SaveAutoSequenceConfigLocked()
{
	Setting().delete_section(AUTO_SEQUENCE_SECTION);
	Setting().write(AUTO_SEQUENCE_SECTION, "MemoryAddress", static_cast<int>(gAutoSequenceAddress));
	Setting().write(AUTO_SEQUENCE_SECTION, "Wrap", gAutoSequenceWrap ? 1 : 0);
	Setting().write(AUTO_SEQUENCE_SECTION, "PlaylistCount", static_cast<int>(gAutoSequencePlaylist.size()));
	Setting().write(AUTO_SEQUENCE_SECTION, "CurrentEntry",
		gAutoSequenceCurrentEntry >= 0 ? gAutoSequenceCurrentEntry + 1 : 0);

	for (size_t i = 0; i < gAutoSequencePlaylist.size() && i < 255; ++i)
	{
		char key[32] = {};
		sprintf(key, "Entry%03u", static_cast<unsigned int>(i + 1));
		Setting().write(AUTO_SEQUENCE_SECTION, key, gAutoSequencePlaylist[i]);
	}
	Setting().flush();
}

static void RegisterAutoSequenceWatch()
{
	if (!SetCpuMemoryWriteWatch) return;
	unsigned short address = 0;
	bool enabled = false;
	{
		std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
		address = gAutoSequenceAddress;
		enabled = !gAutoSequencePlaylist.empty();
	}
	SetCpuMemoryWriteWatch(gSlotId, address, enabled);
}

static bool ReplaceDrive3Image(const std::string& path)
{
	if (!FdcIsIdle())
		return false;

	// Slot 4 is the controller's NONE sentinel.  Never use it as temporary
	// storage: CurrentDisk can legitimately equal NONE while the FDC is idle.
	// Slot 5 is a private staging slot outside the four emulated drives and
	// the NONE sentinel.
	const unsigned char drive3_head_position = gVirtualDrive[3].HeadPosition;
	if (gVirtualDrive[AUTO_SEQUENCE_STAGING_DRIVE].FileHandle != nullptr)
		unmount_disk_image(AUTO_SEQUENCE_STAGING_DRIVE);

	if (mount_disk_image(path.c_str(), AUTO_SEQUENCE_STAGING_DRIVE) == 0)
	{
		unmount_disk_image(AUTO_SEQUENCE_STAGING_DRIVE);
		return false;
	}

	// The physical drive head remains at its existing track while media changes.
	gVirtualDrive[AUTO_SEQUENCE_STAGING_DRIVE].HeadPosition = drive3_head_position;

	// Drive 3 may previously have represented a raw physical device.
	// Once the staged DSK replaces it, clear the old physical-drive mapping.
	if (PhysicalDriveA == 4) PhysicalDriveA = 0;
	if (PhysicalDriveB == 4) PhysicalDriveB = 0;

	std::swap(gVirtualDrive[3], gVirtualDrive[AUTO_SEQUENCE_STAGING_DRIVE]);
	unmount_disk_image(AUTO_SEQUENCE_STAGING_DRIVE); // closes former Drive 3 media
	return true;
}

static void ProcessPendingAutoSequence()
{
	if (!FdcIsIdle())
		return;

	AutoSequenceRequest request{};
	int target = -1;
	std::string target_path;

	{
		std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
		if (gAutoSequenceRequests.empty() || gAutoSequencePlaylist.empty())
			return;

		request = gAutoSequenceRequests.front();
		gAutoSequenceRequests.pop_front();

		const int current_entry = gAutoSequenceCurrentEntry;
		const int playlist_size = static_cast<int>(gAutoSequencePlaylist.size());
		const bool previous_command =
			request.address == static_cast<unsigned short>(gAutoSequenceAddress + 1);

		if (previous_command)
		{
			if (current_entry > 0)
				target = current_entry - 1;
			else if (gAutoSequenceWrap)
				target = playlist_size - 1;
		}
		else if (request.address == gAutoSequenceAddress)
		{
			if (request.value == 0)
			{
				if (current_entry < 0)
					target = 0;
				else if (current_entry + 1 < playlist_size)
					target = current_entry + 1;
				else if (gAutoSequenceWrap)
					target = 0;
			}
			else if (request.value <= playlist_size)
			{
				target = static_cast<int>(request.value) - 1;
			}
		}

		if (target < 0 || target >= playlist_size)
			return;

		target_path = gAutoSequencePlaylist[static_cast<size_t>(target)];
	}

	// Recheck immediately before the media swap. If a command began since the
	// first test, preserve this request for the next idle scanline.
	if (!FdcIsIdle())
	{
		std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
		gAutoSequenceRequests.push_front(request);
		return;
	}

	const bool already_mounted =
		gVirtualDrive[3].FileHandle != nullptr &&
		_stricmp(gVirtualDrive[3].ImageName, target_path.c_str()) == 0;

	if (!already_mounted && !ReplaceDrive3Image(target_path))
	{
		DLOG_C("AutoSequence: could not mount playlist entry %d: %s\n", target + 1, target_path.c_str());
		return;
	}

	{
		std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
		gAutoSequenceCurrentEntry = target;
		SaveAutoSequenceConfigLocked();
	}

	if (PersistDisks)
		Setting().write(MS_FD502, "Disk#3", gVirtualDrive[3].ImageName);

	SendMessage(gVccWnd, WM_VCC_UPD_MENU, (WPARAM)0, (LPARAM)0);
}

static void RefreshAutoSequenceList(HWND hDlg, const AutoSequenceDialogState& state)
{
	SendDlgItemMessage(hDlg, IDC_AS_PLAYLIST, LB_RESETCONTENT, 0, 0);
	for (size_t i = 0; i < state.playlist.size(); ++i)
	{
		char row[MAX_PATH + 16] = {};
		const std::string name = VCC::Util::GetFileNamePart(state.playlist[i]);
		sprintf(row, "%03u  %s", static_cast<unsigned int>(i + 1), name.c_str());
		SendDlgItemMessage(hDlg, IDC_AS_PLAYLIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row));
	}

	char count[48] = {};
	sprintf(count, "%u / 255 entries", static_cast<unsigned int>(state.playlist.size()));
	SetDlgItemText(hDlg, IDC_AS_COUNT, count);
}

static INT_PTR CALLBACK AutoSequenceDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto* state = reinterpret_cast<AutoSequenceDialogState*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));

	switch (message)
	{
	case WM_INITDIALOG:
		state = reinterpret_cast<AutoSequenceDialogState*>(lParam);
		SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
		{
			char address[16] = {};
			sprintf(address, "%04X", state->address);
			SetDlgItemText(hDlg, IDC_AS_ADDRESS, address);
			SendDlgItemMessage(hDlg, IDC_AS_WRAP, BM_SETCHECK, state->wrap ? BST_CHECKED : BST_UNCHECKED, 0);
			RefreshAutoSequenceList(hDlg, *state);
		}
		return TRUE;

	case WM_COMMAND:
		if (!state) return TRUE;
		switch (LOWORD(wParam))
		{
		case IDC_AS_ADD:
			if (state->playlist.size() >= 255)
			{
				MessageBox(hDlg, "The Drive 3 AutoSequence playlist is limited to 255 entries.", "AutoSequence", MB_OK | MB_ICONINFORMATION);
				return TRUE;
			}
			{
				FileDialog dlg;
				dlg.setInitialDir(FloppyPath);
				dlg.setFilter("Disk Images (*.dsk)\0*.dsk\0\0");
				dlg.setDefExt("dsk");
				dlg.setTitle("Add Drive 3 AutoSequence Disk");
				dlg.setFlags(OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST);
				if (dlg.show(0, hDlg))
				{
					char path[MAX_PATH] = {};
					dlg.getpath(path, MAX_PATH);
					if (path[0])
					{
						state->playlist.emplace_back(path);
						dlg.getdir(FloppyPath);
						RefreshAutoSequenceList(hDlg, *state);
						SendDlgItemMessage(hDlg, IDC_AS_PLAYLIST, LB_SETCURSEL,
							static_cast<WPARAM>(state->playlist.size() - 1), 0);
					}
				}
			}
			return TRUE;

		case IDC_AS_REMOVE:
			{
				const LRESULT selected = SendDlgItemMessage(hDlg, IDC_AS_PLAYLIST, LB_GETCURSEL, 0, 0);
				if (selected != LB_ERR && selected < static_cast<LRESULT>(state->playlist.size()))
				{
					state->playlist.erase(state->playlist.begin() + selected);
					RefreshAutoSequenceList(hDlg, *state);
					if (!state->playlist.empty())
					{
						const size_t next = (static_cast<size_t>(selected) < state->playlist.size())
							? static_cast<size_t>(selected) : state->playlist.size() - 1;
						SendDlgItemMessage(hDlg, IDC_AS_PLAYLIST, LB_SETCURSEL, next, 0);
					}
				}
			}
			return TRUE;

		case IDC_AS_UP:
		case IDC_AS_DOWN:
			{
				const LRESULT selected = SendDlgItemMessage(hDlg, IDC_AS_PLAYLIST, LB_GETCURSEL, 0, 0);
				if (selected == LB_ERR) return TRUE;
				const size_t from = static_cast<size_t>(selected);
				const size_t to = (LOWORD(wParam) == IDC_AS_UP)
					? (from == 0 ? from : from - 1)
					: (from + 1 >= state->playlist.size() ? from : from + 1);
				if (from != to)
				{
					std::swap(state->playlist[from], state->playlist[to]);
					RefreshAutoSequenceList(hDlg, *state);
					SendDlgItemMessage(hDlg, IDC_AS_PLAYLIST, LB_SETCURSEL, to, 0);
				}
			}
			return TRUE;

		case IDOK:
			{
				char address_text[32] = {};
				GetDlgItemText(hDlg, IDC_AS_ADDRESS, address_text, sizeof(address_text));
				char* number_text = address_text;
				while (*number_text && isspace(static_cast<unsigned char>(*number_text))) ++number_text;
				if (number_text[0] == '&' && (number_text[1] == 'H' || number_text[1] == 'h'))
					number_text += 2;

				char* end = nullptr;
				const unsigned long parsed = strtoul(number_text, &end, 16);
				while (end && *end && isspace(static_cast<unsigned char>(*end))) ++end;
				if (number_text[0] == '\0' || end == number_text || (end && *end) || parsed > 0xFDFC)
				{
					MessageBox(hDlg, "Enter a hexadecimal address from 0000 to FFFE; the next byte is reserved for AutoSequence status.", "AutoSequence", MB_OK | MB_ICONWARNING);
					return TRUE;
				}
				state->address = static_cast<unsigned short>(parsed);
				state->wrap = (SendDlgItemMessage(hDlg, IDC_AS_WRAP, BM_GETCHECK, 0, 0) == BST_CHECKED);
				EndDialog(hDlg, IDOK);
				return TRUE;
			}

		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

static void OpenAutoSequenceDialog(HWND owner)
{
	AutoSequenceDialogState state;
	{
		std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
		state.playlist = gAutoSequencePlaylist;
		state.address = gAutoSequenceAddress;
		state.wrap = gAutoSequenceWrap;
		if (gAutoSequenceCurrentEntry >= 0 &&
			gAutoSequenceCurrentEntry < static_cast<int>(gAutoSequencePlaylist.size()))
			state.current_path = gAutoSequencePlaylist[static_cast<size_t>(gAutoSequenceCurrentEntry)];
	}

	const INT_PTR result = DialogBoxParam(
		gModuleInstance,
		(LPCTSTR)IDD_AUTOSEQUENCE,
		owner,
		(DLGPROC)AutoSequenceDialogProc,
		reinterpret_cast<LPARAM>(&state));

	if (result != IDOK)
		return;

	{
		std::lock_guard<std::mutex> lock(gAutoSequenceMutex);
		gAutoSequencePlaylist = state.playlist;
		gAutoSequenceAddress = state.address;
		gAutoSequenceWrap = state.wrap;
		gAutoSequenceCurrentEntry = -1;

		if (!state.current_path.empty())
		{
			for (size_t i = 0; i < gAutoSequencePlaylist.size(); ++i)
			{
				if (_stricmp(state.current_path.c_str(), gAutoSequencePlaylist[i].c_str()) == 0)
				{
					gAutoSequenceCurrentEntry = static_cast<int>(i);
					break;
				}
			}
		}
		if (gAutoSequenceCurrentEntry < 0 && gVirtualDrive[3].ImageName[0])
		{
			for (size_t i = 0; i < gAutoSequencePlaylist.size(); ++i)
			{
				if (_stricmp(gVirtualDrive[3].ImageName, gAutoSequencePlaylist[i].c_str()) == 0)
				{
					gAutoSequenceCurrentEntry = static_cast<int>(i);
					break;
				}
			}
		}

		gAutoSequenceRequests.clear();
		SaveAutoSequenceConfigLocked();
		if (FloppyPath[0])
			Setting().write(MS_PATHS, "FloppyPath", FloppyPath);
	}

	RegisterAutoSequenceWatch();
	SendMessage(gVccWnd, WM_VCC_UPD_MENU, (WPARAM)0, (LPARAM)0);
}

void LoadConfig()  // Called on SetIniPath
{
	unsigned char Index=0;
	char Temp[16]="";
	char DiskRomPath[MAX_PATH], RGBRomPath[MAX_PATH];
	char DiskName[MAX_PATH]="";
	unsigned int RetVal=0;

#ifdef COMBINE_BECKER
	BeckerEnabled=Setting().read(MS_BECKER,"DWEnable", 0);
	Setting().read(MS_BECKER,"DWServerAddr","127.0.0.1",BeckerAddr,MAX_PATH);
	Setting().read(MS_BECKER,"DWServerPort","65504",BeckerPort,32);
	if(*BeckerAddr == '\0') strcpy(BeckerAddr,"127.0.0.1");
	if(*BeckerPort == '\0') strcpy(BeckerPort,"65504");
	becker_sethost(BeckerAddr,BeckerPort);
	becker_enable(BeckerEnabled);
#endif

	Setting().read(MS_PATHS, "FloppyPath", "", FloppyPath, MAX_PATH);

	SelectRom = Setting().read(MS_FD502,"DiskRom",1);  //0 External 1=TRSDOS 2=RGB Dos
	Setting().read(MS_FD502,"RomPath","",RomFileName,MAX_PATH);
	PersistDisks=Setting().read(MS_FD502,"Persist",1);
	CheckPath(RomFileName);
	LoadExtRom(External,RomFileName); //JF
	GetModuleFileName(nullptr, DiskRomPath, MAX_PATH);
	PathRemoveFileSpec(DiskRomPath);
	strcpy(RGBRomPath, DiskRomPath);
	strcat(DiskRomPath, "disk11.rom");
	strcat(RGBRomPath, "rgbdos.rom");
	LoadExtRom(TandyDisk, DiskRomPath); // Why load?
	LoadExtRom(RGBDisk, RGBRomPath);    // Why load?
	if (PersistDisks)                   // TODO: Fix this, checkbox was removed
		for (Index=0;Index<4;Index++)
		{
			sprintf(Temp,"Disk#%i",Index);
			Setting().read(MS_FD502,Temp,"",DiskName,MAX_PATH);
			if (strlen(DiskName))
			{
				RetVal=mount_disk_image(DiskName,Index);
				//MessageBox(0, "Disk load attempt", "OK", 0);
				if (RetVal)
				{
					if (!strcmp(DiskName,"*Floppy A:"))	//RealDisks
						PhysicalDriveA=Index+1;
					if (!strcmp(DiskName,"*Floppy B:"))
						PhysicalDriveB=Index+1;
				}
			}
		}
	ClockEnabled=Setting().read(MS_FD502,"ClkEnable",1);
	SetTurboDisk(Setting().read(MS_FD502, "TurboDisk", 1));

	SendMessage(gVccWnd,WM_VCC_UPD_MENU,(WPARAM) 0,(LPARAM) 0);

	return;
}

void SaveConfig()
{
	unsigned char Index=0;
	char Temp[16]="";
	ValidatePath(RomFileName);
	Setting().write(MS_FD502,"DiskRom",SelectRom );
	Setting().write(MS_FD502,"RomPath",RomFileName);
	Setting().write(MS_FD502,"Persist",PersistDisks );
	if (PersistDisks)
		for (Index=0;Index<4;Index++)
		{
			sprintf(Temp,"Disk#%i",Index);
			Setting().write(MS_FD502,Temp,gVirtualDrive[Index].ImageName);
		}
	Setting().write(MS_FD502,"ClkEnable",ClockEnabled );
	Setting().write(MS_FD502, "TurboDisk", SetTurboDisk(QUERY));
	if (strcmp(FloppyPath, "") != 0) {
		Setting().write(MS_PATHS, "FloppyPath", FloppyPath);
	}
#ifdef COMBINE_BECKER
	Setting().write(MS_BECKER,"DWEnable", BeckerEnabled);
	Setting().write(MS_BECKER,"DWServerAddr", BeckerAddr);
	Setting().write(MS_BECKER,"DWServerPort", BeckerPort);
#endif
	return;
}

unsigned char LoadExtRom( unsigned char RomType,const char *FilePath)	//Returns 1 on if loaded
{
	FILE *rom_handle=nullptr;
	unsigned short index=0;
	unsigned char RetVal=0;
	unsigned char *ThisRom[3]={ExternalRom,DiskRom,RGBDiskRom};

	rom_handle=fopen(FilePath,"rb");
	if (rom_handle==nullptr)
		memset(ThisRom[RomType],0xFF,EXTROMSIZE);
	else
	{
		while ((feof(rom_handle)==0) & (index<EXTROMSIZE))
			ThisRom[RomType][index++]=fgetc(rom_handle);
		RetVal=1;
		fclose(rom_handle);
	}

	DLOG_C("load %s %d %d\n",FilePath,RomType,RetVal);
	return RetVal;
}
