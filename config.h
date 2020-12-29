#ifndef __CONFIG_H__
#define __CONFIG_H__
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
#include<iostream>
using namespace std;
void LoadConfig(SystemState *);
unsigned char WriteIniFile(void);
unsigned char ReadIniFile(void);
char * BasicRomName(void);
void GetIniFilePath( char *);
void UpdateConfig (void);
void UpdateSoundBar(unsigned short,unsigned short);
void UpdateTapeCounter(unsigned int,unsigned char);
int GetKeyboardLayout();
void SetWindowSize(POINT);

void SetIniFilePath(char *); //EJJ
char * AppDirectory();       //EJJ

int GetPaletteType();
POINT GetIniWindowSize();
int GetRememberSize();
void SetConfigPath(int, string);

LRESULT CALLBACK	Config			(HWND, UINT, WPARAM, LPARAM);

typedef struct  {
	unsigned char	CPUMultiplyer;
	unsigned short	MaxOverclock;
	unsigned char	FrameSkip;
	unsigned char	SpeedThrottle;
	unsigned char	CpuType;
//	unsigned char	AudioMute;
	unsigned char	MonitorType;
	unsigned char   PaletteType;
	unsigned char	ScanLines;
	unsigned char	Resize;
	unsigned char	Aspect;
	unsigned short	RememberSize;
	unsigned short	WindowSizeX;
	unsigned short	WindowSizeY;
	unsigned char	RamSize;
	unsigned char	AutoStart;
	unsigned char	CartAutoStart;
	unsigned char	RebootNow;
	unsigned char	SndOutDev;
	unsigned char	KeyMap;
	char			SoundCardName[64];
	unsigned short	AudioRate;
	char			ExternalBasicImage[MAX_PATH];
	char			ModulePath[MAX_PATH];
	char			PathtoExe[MAX_PATH];
	char			FloppyPath[MAX_PATH];
	char			CassPath[MAX_PATH];
	char            COCO3ROMPath[MAX_PATH];
} STRConfig;

void DecreaseOverclockSpeed();
void IncreaseOverclockSpeed();

#endif

