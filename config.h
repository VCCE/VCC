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
#include "defines.h"
#include <iostream>

void LoadConfig(SystemState *);
void InitSound();
void LoadModule();
unsigned char WriteIniFile(void);
unsigned char ReadIniFile(void);
void GetIniFilePath( char *);
void UpdateConfig (void);
void UpdateSoundBar(const unsigned int *,unsigned int);
void UpdateTapeCounter(unsigned int,unsigned char,bool force = false);
int GetKeyboardLayout();
void SetWindowRect(const VCC::Rect&);
void CaptureCurrentWindowRect();

void SetIniFilePath(const char *);
void SetKeyMapFilePath(const char *);
char * AppDirectory();
char * GetKeyMapFilePath();

int GetPaletteType();
enum PALETTETYPE {PALETTE_ORIG=0, PALETTE_UPD=1, PALETTE_NTSC=2};

const VCC::Rect& GetIniWindowRect();
int GetRememberSize();
void SwapJoySticks();

void DecreaseOverclockSpeed();
void IncreaseOverclockSpeed();
void SetOverclock(unsigned char);

// Openers for config dialogs
void OpenAudioConfig();
void OpenCpuConfig();
void OpenDisplayConfig();
void OpenInputConfig();
void OpenJoyStickConfig();
void OpenTapeConfig();
void OpenBitBangerConfig();

// For tcc1014mmu to load coco3 rom
void GetExtRomPath(char *);

#endif

