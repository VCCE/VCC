////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////

//#define USE_LOGGING
#include "resource.h"
#include "wd1793.h"
#include "fd502.h"
#include "utils.h"
#include "vcc/ui/menu/menu_builder.h"
#include "vcc/devices/serial/beckerport.h"
#include "vcc/devices/rtc/oki_m6242b.h"
#include "vcc/devices/rom/rom_image.h"
#include "vcc/utils/FileOps.h"
#include "vcc/utils/logger.h"
#include "vcc/utils/winapi.h"
#include "vcc/utils/persistent_value_store.h"
#include "vcc/utils/filesystem.h"
#include "vcc/common/DialogOps.h"
#include <Windows.h>
#include <array>


extern HINSTANCE gModuleInstance;

static const auto default_selected_rom_index = 1u;

static std::string FloppyPath;
static size_t SelectRomIndex = default_selected_rom_index;
static std::string RomFileName;
static auto PersistDisks = false;
static auto ClockEnabled = true;

static auto BeckerEnabled = false;
static std::string BeckerAddr;
static std::string BeckerPort;


static ::vcc::devices::rom::rom_image ExternalRom;
static ::vcc::devices::rom::rom_image DiskRom;
static ::vcc::devices::rom::rom_image RGBDiskRom;

unsigned char PhysicalDriveA = 0;
unsigned char PhysicalDriveB = 0;
unsigned char OldPhysicalDriveA = 0;
unsigned char OldPhysicalDriveB = 0;
static const std::array<::vcc::devices::rom::rom_image*, 3> RomPointer = { &ExternalRom, &DiskRom, &RGBDiskRom };
static unsigned char NewDiskNumber = 0;
static unsigned char CreateFlag = 0;
static char IniFile[MAX_PATH]="";
static size_t TempSelectRomIndex = default_selected_rom_index;

static HWND g_hConfDlg = nullptr;
static unsigned long RealDisks=0;
long CreateDisk (unsigned char);
static char TempFileName[MAX_PATH]="";
static char TempRomFileName[MAX_PATH]="";


static ::vcc::devices::rtc::oki_m6242b gDistoRtc;
static ::vcc::devices::serial::Becker gBecker;

unsigned char SetChip(unsigned char);

LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK NewDisk(HWND,UINT, WPARAM, LPARAM);
void LoadConfig();
void SaveConfig();
void Load_Disk(unsigned char);




//----------------------------------------------------------------------------


fd502_cartridge::fd502_cartridge(
	std::shared_ptr<expansion_port_host_type> host,
	std::unique_ptr<expansion_port_ui_type> ui,
	std::shared_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	host_(host),
	ui_(move(ui)),
	bus_(bus),
	module_instance_(module_instance),
	driver_(host, bus)
{}


fd502_cartridge::name_type fd502_cartridge::name() const
{
	return ::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME);
}

fd502_cartridge::driver_type& fd502_cartridge::driver()
{
	return driver_;
}


void fd502_cartridge::start()
{
	strcpy(IniFile, host_->configuration_path().c_str());

	RealDisks = InitController();
	LoadConfig();
}

void fd502_cartridge::stop()
{
	CloseCartDialog(g_hConfDlg);
	for (unsigned char Drive = 0; Drive <= 3; Drive++)
	{
		unmount_disk_image(Drive);
	}
}


void fd502_cartridge::menu_item_clicked(unsigned char MenuID)
{
	HWND h_own = GetActiveWindow();
	switch (MenuID)
	{
		case 10:
			Load_Disk(0);
			break;
		case 11:
			unmount_disk_image(0);
			SaveConfig();
			break;
		case 12:
			Load_Disk(1);
			break;
		case 13:
			unmount_disk_image(1);
			SaveConfig();
			break;
		case 14:
			Load_Disk(2);
			break;
		case 15:
			unmount_disk_image(2);
			SaveConfig();
			break;
		case 16:
			if (g_hConfDlg == nullptr)
				g_hConfDlg = CreateDialog(gModuleInstance,(LPCTSTR)IDD_CONFIG,h_own,(DLGPROC)Config);
			ShowWindow(g_hConfDlg,1);
			break;
		case 17:
			Load_Disk(3);
			break;
		case 18:
			unmount_disk_image(3);
			SaveConfig();
			break;
	}
}

void fd502_cartridge_driver::write_port(unsigned char Port, unsigned char Data)
{
	if (BeckerEnabled && Port == 0x42)
	{
		gBecker.write(Data, Port);
	}
	else if (ClockEnabled && Port == 0x50)
	{
		gDistoRtc.write_data(Data);
	}
	else if (ClockEnabled && Port == 0x51)
	{
		gDistoRtc.set_read_write_address(Data);
	}
	else
	{
		disk_io_write(*bus_, Data, Port);
	}
}

unsigned char fd502_cartridge_driver::read_port(unsigned char Port)
{
	if (BeckerEnabled && (Port == 0x41 || Port == 0x42))
	{
		return(gBecker.read(Port));
	}

	if (ClockEnabled && (Port == 0x50 || Port == 0x51))
	{
		return gDistoRtc.read_data();
	}

	return disk_io_read(*bus_, Port);
}

void fd502_cartridge_driver::update(float delta)
{
	PingFdc(*bus_);
}

unsigned char fd502_cartridge_driver::read_memory_byte(size_type Address)
{
	return RomPointer[SelectRomIndex]->read_memory_byte(Address);
}

void fd502_cartridge::status(char* text_buffer, size_t buffer_size)
{
	char diskstat[64];
	DiskStatus(diskstat, sizeof(diskstat));
	strcpy(text_buffer,diskstat);
	char beckerstat[64];
	if(BeckerEnabled) {
		gBecker.status(beckerstat);
		strcat(text_buffer," | ");
		strcat(text_buffer,beckerstat);
	}
	return ;
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	static unsigned char temp=0,temp2=0;
	long ChipChoice[3]={IDC_EXTROM,IDC_TRSDOS,IDC_RGB};
	long VirtualDrive[2]={IDC_DISKA,IDC_DISKB};
	char VirtualNames[5][16]={"None","Drive 0","Drive 1","Drive 2","Drive 3"};

	switch (message)
	{
		case WM_CLOSE:
			DestroyWindow(hDlg);
			g_hConfDlg = nullptr;
			return TRUE;

		case WM_INITDIALOG:
			CenterDialog(hDlg);
			TempSelectRomIndex = SelectRomIndex;
			if (!RealDisks)
			{
				EnableWindow(GetDlgItem(hDlg, IDC_DISKA), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_DISKB), FALSE);
				PhysicalDriveA=0;
				PhysicalDriveB=0;
			}
			SendDlgItemMessage(hDlg, IDC_CLOCK, BM_SETCHECK, ClockEnabled, false);
			OldPhysicalDriveA=PhysicalDriveA;
			OldPhysicalDriveB=PhysicalDriveB;
			strcpy(TempRomFileName,RomFileName.c_str());
			SendDlgItemMessage(hDlg,IDC_TURBO,BM_SETCHECK,SetTurboDisk(QUERY),0);
			SendDlgItemMessage(hDlg, IDC_PERSIST, BM_SETCHECK, PersistDisks, false);
			for (temp = 0; temp < RomPointer.size(); temp++)
			{
				SendDlgItemMessage(hDlg, ChipChoice[temp], BM_SETCHECK, (temp == TempSelectRomIndex), 0);
			}
			for (temp=0;temp<2;temp++)
				for (temp2=0;temp2<5;temp2++)
						SendDlgItemMessage (hDlg,VirtualDrive[temp], CB_ADDSTRING, 0, (LPARAM) VirtualNames[temp2]);

			SendDlgItemMessage (hDlg, IDC_DISKA,CB_SETCURSEL,(WPARAM)PhysicalDriveA,(LPARAM)0);
			SendDlgItemMessage (hDlg, IDC_DISKB,CB_SETCURSEL,(WPARAM)PhysicalDriveB,(LPARAM)0);
			SendDlgItemMessage (hDlg,IDC_ROMPATH,WM_SETTEXT,0,(LPARAM)(LPCSTR)TempRomFileName);

			SendDlgItemMessage(hDlg, IDC_BECKER_ENAB, BM_SETCHECK, BeckerEnabled, false);
			SendDlgItemMessage (hDlg,IDC_BECKER_HOST,WM_SETTEXT,0,reinterpret_cast<const LPARAM>(BeckerAddr.c_str()));
			SendDlgItemMessage (hDlg,IDC_BECKER_PORT,WM_SETTEXT,0,reinterpret_cast<const LPARAM>(BeckerPort.c_str()));

			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					ClockEnabled = SendDlgItemMessage(hDlg, IDC_CLOCK, BM_GETCHECK, 0, 0) != 0;
					SetTurboDisk((unsigned char)SendDlgItemMessage(hDlg,IDC_TURBO,BM_GETCHECK,0,0));
					PersistDisks = SendDlgItemMessage(hDlg, IDC_PERSIST, BM_GETCHECK, 0, 0) != 0;
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
					SelectRomIndex = std::min(TempSelectRomIndex, RomPointer.size());
					RomFileName = ::vcc::utils::find_pak_module_path(TempRomFileName);
					ExternalRom.load(RomFileName); //JF
					BeckerAddr = ::vcc::utils::get_dialog_item_text(hDlg, IDC_BECKER_HOST);
					BeckerPort = ::vcc::utils::get_dialog_item_text(hDlg, IDC_BECKER_PORT);
					gBecker.sethost(BeckerAddr.c_str(), BeckerPort.c_str());
					BeckerEnabled = SendDlgItemMessage(hDlg, IDC_BECKER_ENAB, BM_GETCHECK, 0, 0) != 0;
					gBecker.enable(BeckerEnabled);
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
							TempSelectRomIndex = temp;
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

void Load_Disk(unsigned char disk)
{
	HWND h_own = GetActiveWindow();
	FileDialog dlg;
	dlg.setInitialDir(FloppyPath.c_str());
	dlg.setFilter("Disk Images\0*.dsk;*.os9\0\0");
	dlg.setDefExt("dsk");
	dlg.setTitle("Insert Disk Image");
	dlg.setFlags(OFN_PATHMUSTEXIST);
	if (dlg.show(0,h_own)) {
		CreateFlag = 1;
		HANDLE hr = nullptr;
		dlg.getpath(TempFileName,MAX_PATH);
		// Verify file exists
		hr = CreateFile
			(TempFileName,0,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
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
			if (mount_disk_image(TempFileName,disk)==0) {
				MessageBox(g_hConfDlg,"Can't open file","Error",0);
			} else {
				FloppyPath = ::vcc::utils::get_directory_from_path(dlg.path());
				SaveConfig();
			}
		}
	}
	return;
}

fd502_cartridge::menu_item_collection_type fd502_cartridge::get_menu_items() const
{
	char TempMsg[64]="";
	char TempBuf[MAX_PATH]="";

	::vcc::ui::menu::menu_builder builder;

	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf, get_mounted_disk_filename(0).c_str());
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	builder
		.add_root_submenu("FD-502 Drive 0")
		.add_submenu_item(10, "Insert")
		.add_submenu_item(11, TempMsg);

	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf, get_mounted_disk_filename(1).c_str());
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	builder
		.add_root_submenu("FD-502 Drive 1")
		.add_submenu_item(12, "Insert")
		.add_submenu_item(13, TempMsg);

	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf, get_mounted_disk_filename(2).c_str());
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	builder
		.add_root_submenu("FD-502 Drive 2")
		.add_submenu_item(14,"Insert")
		.add_submenu_item(15,TempMsg);

	strcpy(TempMsg,"Eject: ");
	strcpy(TempBuf, get_mounted_disk_filename(3).c_str());
	PathStripPath(TempBuf);
	strcat(TempMsg,TempBuf);
	builder
		.add_root_submenu("FD-502 Drive 3")
		.add_submenu_item(17, "Insert")
		.add_submenu_item(18, TempMsg);

	return builder
		.add_root_item(16, "FD-502 Config")
		.release_items();
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
			strcpy(Dummy,TempFileName);
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
					const char *ext = TempFileName + strlen(TempFileName) - 4;
					if (ext < TempFileName) ext = TempFileName;
					// if it doesn't end with .dsk, append it
					if (_stricmp(ext, ".dsk") != 0)
					{
						PathRemoveExtension(TempFileName);
						strcat(TempFileName, ".dsk");
					}
					if (CreateDiskHeader(TempFileName, NewDiskType, NewDiskTracks, DblSided))
					{
						strcpy(TempFileName, "");
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

static const auto becker_section_name = "HDBDOS/DW/Becker";
static const auto fd502_section_name = "FD-502";


void LoadConfig()  // Called on SetIniPath
{
	namespace utils = ::vcc::utils;

	char DiskRomPath[MAX_PATH];
	char RGBRomPath[MAX_PATH];

	::vcc::utils::persistent_value_store settings(IniFile);

	BeckerEnabled = settings.read(becker_section_name, "DWEnable", false);
	BeckerAddr = settings.read(becker_section_name, "DWServerAddr", "127.0.0.1");
	BeckerPort = settings.read(becker_section_name, "DWServerPort", "65504");
	gBecker.sethost(BeckerAddr.c_str(), BeckerPort.c_str());
	gBecker.enable(BeckerEnabled);

	FloppyPath = settings.read("DefaultPaths", "FloppyPath");
	SelectRomIndex = std::min(
		settings.read(fd502_section_name, "DiskRom", default_selected_rom_index),
		RomPointer.size());
	RomFileName = utils::find_pak_module_path(settings.read(fd502_section_name, "RomPath"));
	PersistDisks = settings.read(fd502_section_name, "Persist", true);

	RomFileName = utils::find_pak_module_path(RomFileName);
	ExternalRom.load(RomFileName); //JF

	GetModuleFileName(nullptr, DiskRomPath, MAX_PATH);
	PathRemoveFileSpec(DiskRomPath);
	strcpy(RGBRomPath, DiskRomPath);
	strcat(DiskRomPath, "disk11.rom"); //Failing silent, Maybe we should throw a warning?
	strcat(RGBRomPath, "rgbdos.rom");  //Future, Grey out dialog option if can't find file
	DiskRom.load(DiskRomPath);
	RGBDiskRom.load(RGBRomPath);

	if (PersistDisks)
	{
		for (auto Index = 0; Index < 4; Index++)
		{
			const auto settings_key = "Disk#" + std::to_string(Index);
			const auto disk_filename(settings.read(fd502_section_name, settings_key));
			if (!disk_filename.empty())
			{
				if (mount_disk_image(disk_filename.c_str(), Index))
				{
					if (disk_filename == "*Floppy A:")	//RealDisks
					{
						PhysicalDriveA = Index + 1;
					}

					if (disk_filename == "*Floppy B:")
					{
						PhysicalDriveB = Index + 1;
					}
				}
			}
		}
	}

	ClockEnabled = settings.read(fd502_section_name, "ClkEnable", true);
	SetTurboDisk(GetPrivateProfileInt(fd502_section_name, "TurboDisk", 1, IniFile));
}

void SaveConfig()
{
	unsigned char Index = 0;
	char Temp[16] = "";

	::vcc::utils::persistent_value_store settings(IniFile);

	RomFileName = ::vcc::utils::strip_application_path(RomFileName);
	settings.write(fd502_section_name, "DiskRom", SelectRomIndex);
	settings.write(fd502_section_name, "RomPath", RomFileName);
	settings.write(fd502_section_name, "Persist", PersistDisks);
	if (PersistDisks)
	{
		for (auto disk_id = 0; disk_id < 4; disk_id++)
		{
			sprintf(Temp, "Disk#%i", disk_id);
			settings.write(fd502_section_name, Temp, get_mounted_disk_filename(disk_id));
		}
	}
	settings.write(fd502_section_name, "ClkEnable", ClockEnabled);
	settings.write(fd502_section_name, "TurboDisk", SetTurboDisk(QUERY));
	if (!FloppyPath.empty())
	{
		settings.write("DefaultPaths", "FloppyPath", FloppyPath);
	}

	settings.write(becker_section_name, "DWEnable", BeckerEnabled);
	settings.write(becker_section_name, "DWServerAddr", BeckerAddr);
	settings.write(becker_section_name, "DWServerPort", BeckerPort);
}
