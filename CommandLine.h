#ifndef __COMMANDLINE_H__
#define __COMMANDLINE_H__
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

// Declare global variables defined by GetCmdLineArgs
constexpr auto CL_MAX_PATH = 256u;
constexpr auto CL_MAX_PASTE = 256u;
struct CmdLineArguments {
	char QLoadFile[CL_MAX_PATH];
	char IniFile[CL_MAX_PATH];
	char PasteText[CL_MAX_PASTE];
	int  Logging;
};
extern struct CmdLineArguments CmdArg;

// Get Settings from Command line string 
int  GetCmdLineArgs(char * lpCmdLine);

// Errors returned
// FIXME: These need to be turned into a scoped enum and the signature of functions
// that use them updated.
#define CL_ERR_UNKOPT 1  // Unknown option found
#define CL_ERR_XTRARG 2  // Too many arguments

#endif
