#ifndef __COMMANDLINE_H__
#define __COMMANDLINE_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
Copyright 2015 by E J Jaquay
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


#include <windows.h>  // For MAX_PATH

// Global variables defined by CommandLineSettings

char QuickLoadFile[MAX_PATH];
char IniFilePath[MAX_PATH];
int  ConsoleLogging;

// Get Settings from Command line string (lpCmdLine from WinMain)
int  CommandLineSettings(char *);

#ifdef __cplusplus
}
#endif

#endif

