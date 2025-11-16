
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
//
//----------------------------------------------------------------------
// SDC simulator E J Jaquay 2025 
//----------------------------------------------------------------------
//
#pragma once
#include "vcc/devices/psg/sn76496.h"
#include "vcc/devices/rom/banked_rom_image.h"
#include "vcc/bus/cartridge.h"
#include "vcc/bus/cartridge_context.h"
#include <memory>
#include <Windows.h>

//======================================================================
// Functions
//======================================================================

static ::vcc::devices::rtc::ds1315 ds1315_rtc;

void LoadRom(unsigned char);
void MemWrite(unsigned char,unsigned short);
unsigned char MemRead(unsigned short);
LRESULT CALLBACK SDC_Control(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SDC_Configure(HWND, UINT, WPARAM, LPARAM);

void CloseDrive(int);
bool IsDirectory(const char *);

std::string DiskName(int);

//======================================================================
// Public Data
//======================================================================

// Idle Status counter
extern int idle_ctr;

// Cart ROM
extern char PakRom[0x4000];

// Host paths for SDC
extern char IniFile[MAX_PATH];  // Vcc ini file name
extern char SDCard[MAX_PATH];   // SD card root directory

// Flash banks
extern char FlashFile[8][MAX_PATH];
extern unsigned char StartupBank;
extern unsigned char CurrentBank;
extern unsigned char EnableBankWrite;
extern unsigned char BankWriteState;

// Clock enable IDC_CLOCK
extern int ClockEnable;

extern char SDC_Status[16];




class sdc_cartridge : public ::vcc::bus::cartridge
{
public:

	using context_type = ::vcc::bus::cartridge_context;
	using path_type = std::string;
	using rom_image_type = ::vcc::devices::rom::banked_rom_image;


public:

	sdc_cartridge(std::unique_ptr<context_type> context, HINSTANCE module_instance);

	name_type name() const override;
	catalog_id_type catalog_id() const override;
	description_type description() const override;

	void start() override;
	void stop() override;
	void reset() override;

	[[nodiscard]] unsigned char read_memory_byte(size_type memory_address) override;

	void write_port(unsigned char port_id, unsigned char value) override;
	unsigned char read_port(unsigned char port_id) override;

	void status(char* status_buffer, size_t buffer_size) override;

	void menu_item_clicked(unsigned char menu_item_id) override;


private:

	virtual void SDCInit();

	virtual void BuildCartridgeMenu();
	virtual bool MountNext(int drive);

	virtual void CommandDone();
	virtual void FloppyWriteData(unsigned char data);
	virtual void SDCWrite(unsigned char data, unsigned char port);
	virtual void ParseStartup();
	virtual void SDCCommand();
	virtual void ReadSector();
	virtual void StreamImage();
	virtual void WriteSector();
	virtual bool SeekSector(unsigned char,unsigned int);
	virtual bool ReadDrive(unsigned char,unsigned int);
	virtual void GetDriveInfo();
	virtual void SDCControl();
	virtual void UpdateSD();
	virtual void AppendPathChar(char *,char c);
	virtual bool LoadFoundFile(struct FileRecord *);
	virtual void MountDisk(int,const char *,int);
	virtual void MountNewDisk(int,const char *,int);
	virtual void OpenNew(int,const char *,int);
	virtual void OpenFound(int,int);
	virtual void LoadReply(const void *, int);
	virtual void BlockReceive(unsigned char);
	virtual char * LastErrorTxt();
	virtual void FlashControl(unsigned char);
	virtual void LoadDirPage();
	virtual void SetCurDir(const char *);
	virtual bool SearchFile(const char *);
	virtual void InitiateDir(const char *);
	virtual void GetFullPath(char *,const char *);
	virtual void RenameFile(const char *);
	virtual void KillFile(const char *);
	virtual void MakeDirectory(const char *);
	virtual void GetMountedImageRec();
	virtual void GetSectorCount();
	virtual void GetDirectoryLeaf();
	virtual unsigned char PickReplyByte(unsigned char);
	virtual void FloppyCommand(unsigned char);
	virtual void FloppyRestore(unsigned char);
	virtual void FloppySeek(unsigned char);
	virtual void FloppyReadDisk();
	virtual void FloppyWriteDisk();
	virtual void FloppyTrack(unsigned char);
	virtual void FloppySector(unsigned char);
	virtual unsigned char FloppyStatus();
	virtual unsigned char FloppyReadData();
	virtual unsigned char SDCRead(unsigned char port);
	virtual unsigned char WriteFlashBank(unsigned short adr);


private:

	const std::unique_ptr<context_type> context_;
	const HINSTANCE module_instance_;
};
