
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

#include <Windows.h>

//======================================================================
// Functions
//======================================================================

static ::vcc::devices::rtc::ds1315 ds1315_rtc;

void SDCInit();
void LoadRom(unsigned char);
void SDCWrite(unsigned char,unsigned char);
void MemWrite(unsigned char,unsigned short);
unsigned char SDCRead(unsigned char port);
unsigned char MemRead(unsigned short);
LRESULT CALLBACK SDC_Control(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SDC_Configure(HWND, UINT, WPARAM, LPARAM);

void CloseDrive(int);
bool IsDirectory(const char *);
unsigned char WriteFlashBank(unsigned short);
bool MountNext(int);

std::string DiskName(int);

//======================================================================
// Public Data
//======================================================================

extern void* gHostKey;
extern PakAssertInteruptHostCallback AssertInt;
extern PakWriteMemoryByteHostCallback MemWrite8;
extern PakReadMemoryByteHostCallback MemRead8;
extern PakAppendCartridgeMenuHostCallback CartMenuCallback;

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

