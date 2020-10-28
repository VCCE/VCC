#ifndef __VCC_H__
#define __VCC_H__

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

// define this to make the main config dialog modal
// vs. modeless where you can interact with the main window
// while the config dialog is open
//#define CONFIG_DIALOG_MODAL

void shutdown(void);
void SetCPUMultiplyerFlag (unsigned char);
void SetTurboMode(unsigned char);
unsigned char SetCPUMultiplyer(unsigned char );
unsigned char SetRamSize(unsigned char);
unsigned char SetSpeedThrottle(unsigned char);
unsigned char SetFrameSkip(unsigned char);
unsigned char SetCpuType( unsigned char);
unsigned char SetAutoStart(unsigned char);
void SetPaletteType(void);
//void StartCart(char *);
void DoReboot(void);
void DoHardReset(SystemState *);
void LoadPack (void);
void DynamicMenuCallback( char *,int,int);
//void RefreshWindow(void);

//Type 0= HEAD TAG 1= Slave Tag 2=StandAlone
#define	HEAD 0
#define SLAVE 1
#define STANDALONE 2

#endif
