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
#include "SuperIDE.h"
#include <Windows.h>
#include "stdio.h"
#include <iostream>
#include "resource.h" 
#include "IdeBus.h"
#include "vcc/devices/rtc/ds1315.h"
#include "logger.h"
#include "vcc/utils/FileOps.h"
#include "../CartridgeMenu.h"
#include "vcc/common/DialogOps.h"
#include "vcc/utils/winapi.h"

static char FileName[MAX_PATH] { 0 };
static char IniFile[MAX_PATH]  { 0 };
static char SuperIDEPath[MAX_PATH];
static ::vcc::devices::rtc::ds1315 ds1315_rtc;
static unsigned char BaseAddress=0x50;
static LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM );
static void Select_Disk(unsigned char);
static void SaveConfig();
static void LoadConfig();
static unsigned char BaseTable[4]={0x40,0x50,0x60,0x70};

static unsigned char BaseAddr=1;
static bool ClockEnabled = true;
static bool ClockReadOnly = true;
static unsigned char DataLatch=0;
static HWND hConfDlg = nullptr;
static const char* const gConfigurationSection = "Glenside-IDE /w Clock";


superide_cartridge::superide_cartridge(
	std::unique_ptr<expansion_port_host_type> host,
	std::unique_ptr<expansion_port_ui_type> ui,
	std::unique_ptr<expansion_port_bus_type> bus,
	HINSTANCE module_instance)
	:
	host_(move(host)),
	ui_(move(ui)),
	bus_(move(bus)),
	module_instance_(module_instance)
{
}

superide_cartridge::name_type superide_cartridge::name() const
{
	static const auto name(::vcc::utils::load_string(module_instance_, IDS_MODULE_NAME));

	return name.c_str();
}

superide_cartridge::catalog_id_type superide_cartridge::catalog_id() const
{
	static const auto catalog_id(::vcc::utils::load_string(module_instance_, IDS_CATNUMBER));

	return catalog_id.c_str();
}

superide_cartridge::description_type superide_cartridge::description() const
{
	static const auto description(::vcc::utils::load_string(module_instance_, IDS_DESCRIPTION));

	return description.c_str();
}

void superide_cartridge::start()
{
	strcpy(IniFile, host_->configuration_path().c_str());

	LoadConfig();
	IdeInit();

	BuildCartridgeMenu();		
}

void superide_cartridge::stop()
{
	if (hConfDlg)
	{
		CloseCartDialog(hConfDlg);
		hConfDlg = nullptr;
	}
}

void superide_cartridge::write_port(unsigned char Port, unsigned char Data)
{
	if ( (Port >=BaseAddress) & (Port <= (BaseAddress+8)))
		switch (Port-BaseAddress)
		{
			case 0x0:	
				IdeRegWrite(Port-BaseAddress,(DataLatch<<8)+ Data );
				break;

			case 0x8:
				DataLatch=Data & 0xFF;		//Latch
				break;

			default:
				IdeRegWrite(Port-BaseAddress,Data);
				break;
		}	//End port switch	
	return;
}

unsigned char superide_cartridge::read_port(unsigned char Port)
{
	unsigned char RetVal=0;
	unsigned short Temp=0;
	if (((Port == 0x78) | (Port == 0x79) | (Port == 0x7C)) & ClockEnabled)
		RetVal = ds1315_rtc.read_port(Port);

	if ( (Port >=BaseAddress) & (Port <= (BaseAddress+8)))
		switch (Port-BaseAddress)
		{
			case 0x0:	
				Temp=IdeRegRead(Port-BaseAddress);

				RetVal=Temp & 0xFF;
				DataLatch= Temp>>8;
				break;

			case 0x8:
				RetVal=DataLatch;		//Latch
				break;
			default:
				RetVal=IdeRegRead(Port-BaseAddress)& 0xFF;
				break;

		}	//End port switch	
			
	return RetVal;
}

void superide_cartridge::status(char* text_buffer, size_t buffer_size)
{
	DiskStatus(text_buffer, buffer_size);
}

void superide_cartridge::menu_item_clicked(unsigned char item_id)
{
	switch (item_id)
	{
	case 10:
		Select_Disk(MASTER);
		BuildCartridgeMenu();
		SaveConfig();
		break;

	case 11:
		DropDisk(MASTER);
		BuildCartridgeMenu();
		SaveConfig();
		break;

	case 12:
		Select_Disk(SLAVE);
		BuildCartridgeMenu();
		SaveConfig();
		break;

	case 13:
		DropDisk(SLAVE);
		BuildCartridgeMenu();
		SaveConfig();
		break;

	case 14:
		CreateDialog( module_instance_, (LPCTSTR) IDD_CONFIG,
				GetActiveWindow(), (DLGPROC) Config );
		ShowWindow(hConfDlg,1);
		break;
	}
	return;
}


void superide_cartridge::BuildCartridgeMenu()
{
	char TempMsg[512]="";
	char TempBuf[MAX_PATH]="";
	ui_->add_menu_item("", MID_BEGIN, MIT_Head);
	ui_->add_menu_item("", MID_ENTRY, MIT_Seperator);
	ui_->add_menu_item("IDE Master",MID_ENTRY,MIT_Head);
	ui_->add_menu_item("Insert",ControlId(10),MIT_Slave);
	QueryDisk(MASTER,TempBuf);
	strcpy(TempMsg,"Eject: ");
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	ui_->add_menu_item(TempMsg,ControlId(11),MIT_Slave);
	ui_->add_menu_item("IDE Slave",MID_ENTRY,MIT_Head);
	ui_->add_menu_item("Insert",ControlId(12),MIT_Slave);
	QueryDisk(SLAVE,TempBuf);
	strcpy(TempMsg,"Eject: ");
	PathStripPath (TempBuf);
	strcat(TempMsg,TempBuf);
	ui_->add_menu_item(TempMsg,ControlId(13),MIT_Slave);
	ui_->add_menu_item("IDE Config",ControlId(14),MIT_StandAlone);
	ui_->add_menu_item("", MID_FINISH, MIT_Head);
}

LRESULT CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	unsigned char BTemp=0;
	switch (message)
	{
		case WM_INITDIALOG:
			hConfDlg=hDlg;
			SendDlgItemMessage(hDlg, IDC_CLOCK, BM_SETCHECK, ClockEnabled, 0);
			SendDlgItemMessage(hDlg, IDC_READONLY, BM_SETCHECK, ClockReadOnly, 0);
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "40");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "50");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "60");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_ADDSTRING, 0, (LPARAM) "70");
			SendDlgItemMessage(hDlg,IDC_BASEADDR, CB_SETCURSEL,BaseAddr,(LPARAM)0);
			EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
			if (BaseAddr==3)
			{
				ClockEnabled = false;
				SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
				EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), FALSE);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					BaseAddr=(unsigned char)SendDlgItemMessage(hDlg,IDC_BASEADDR,CB_GETCURSEL,0,0);
					ClockEnabled = (unsigned char)SendDlgItemMessage(hDlg, IDC_CLOCK, BM_GETCHECK, 0, 0) != 0;
					ClockReadOnly = (unsigned char)SendDlgItemMessage(hDlg, IDC_READONLY, BM_GETCHECK, 0, 0) != 0;
					BaseAddress=BaseTable[BaseAddr & 3];
					ds1315_rtc.set_read_only(ClockReadOnly);
					SaveConfig();
					EndDialog(hDlg, LOWORD(wParam));

					break;

				case IDCANCEL:
					EndDialog(hDlg, LOWORD(wParam));
					break;

				case IDC_CLOCK:

					break;

				case IDC_READONLY:
					break;

				case IDC_BASEADDR:
					BTemp=(unsigned char)SendDlgItemMessage(hDlg,IDC_BASEADDR,CB_GETCURSEL,0,0);
					EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), TRUE);
					if (BTemp==3)
					{
						ClockEnabled = false;
						SendDlgItemMessage(hDlg,IDC_CLOCK,BM_SETCHECK,ClockEnabled,0);
						EnableWindow(GetDlgItem(hDlg, IDC_CLOCK), FALSE);
					}
					break;
			}
	}
	return 0;
}

void Select_Disk(unsigned char Disk)
{
	FileDialog dlg;
	dlg.setDefExt("IMG");
	dlg.setTitle("Mount IDE hard Disk Image");
	dlg.setFilter("Hard Disk Images\0*.img;*.vhd;*.os9\0All files\0*.*\0\0");
	dlg.setInitialDir(SuperIDEPath);
	if (dlg.show()) {
		if (MountDisk(dlg.path(),Disk)) {
			dlg.getdir(SuperIDEPath);
		} else {
			MessageBox(GetActiveWindow(),"Can't Open Image","Error",0);
		}
	}
	return;
}

void SaveConfig()
{
	QueryDisk(MASTER,FileName);
	WritePrivateProfileString(gConfigurationSection,"Master",FileName,IniFile);
	QueryDisk(SLAVE,FileName);
	WritePrivateProfileString(gConfigurationSection,"Slave",FileName,IniFile);
	WritePrivateProfileInt(gConfigurationSection,"BaseAddr",BaseAddr ,IniFile);
	WritePrivateProfileInt(gConfigurationSection,"ClkEnable",ClockEnabled ,IniFile);
	WritePrivateProfileInt(gConfigurationSection, "ClkRdOnly", ClockReadOnly, IniFile);
	if (strcmp(SuperIDEPath, "") != 0) { 
		WritePrivateProfileString("DefaultPaths", "SuperIDEPath", SuperIDEPath, IniFile); 
	}

	return;
}

void superide_cartridge::LoadConfig()
{
	GetPrivateProfileString("DefaultPaths", "SuperIDEPath", "", SuperIDEPath, MAX_PATH, IniFile);
	GetPrivateProfileString(gConfigurationSection,"Master","",FileName,MAX_PATH,IniFile);
	MountDisk(FileName ,MASTER);
	GetPrivateProfileString(gConfigurationSection,"Slave","",FileName,MAX_PATH,IniFile);
	BaseAddr=GetPrivateProfileInt(gConfigurationSection,"BaseAddr",1,IniFile); 
	ClockEnabled = GetPrivateProfileInt(gConfigurationSection, "ClkEnable", true, IniFile) != 0;
	ClockReadOnly = GetPrivateProfileInt(gConfigurationSection, "ClkRdOnly", true, IniFile) != 0;
	BaseAddr&=3;
	if (BaseAddr == 3)
		ClockEnabled = false;
	BaseAddress=BaseTable[BaseAddr];
	ds1315_rtc.set_read_only(ClockReadOnly);
	MountDisk(FileName ,SLAVE);
	BuildCartridgeMenu();
	return;
}
