#ifndef __DIRECTDRAWINTERFACE_H__
#define __DIRECTDRAWINTERFACE_H__
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

BOOL InitInstance(HINSTANCE,int);
BOOL InitDrawSurface(bool );
void UnlockScreen(SystemState *);
unsigned char LockScreen(SystemState *);
void SetStatusBarText( char *,SystemState *);
int GetMainWindowStatusBarHeight();
bool CreateDDWindow(SystemState *);
void Cls(unsigned int,SystemState *);
void DoCls(SystemState *);
unsigned char SetInfoBand( unsigned char);
unsigned char SetResize(unsigned char);
unsigned char SetAspect (unsigned char);
float Static(SystemState *);
POINT GetCurWindowSize();
void DisplayFlip(SystemState *);


#define MAX_LOADSTRING 100

#endif
