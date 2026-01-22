#pragma once
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

#include <vcc/bus/cpak_cartridge_definitions.h>
#include <vcc/util/Settings.h>

extern void*const& gCallbackContext;
extern PakAssertInteruptHostCallback AssertInt;
void BuildCartridgeMenu();

// Access settings object
VCC::Util::settings& Setting();

// DO NOT REMOVE this or references to ituntil becker.dll is retired.
// Then FD502 becker becomes permanant.
#define COMBINE_BECKER

// FIXME: These need to be turned into a scoped enum and the signature of functions
// that use them updated.
#define External 0
#define TandyDisk 1
#define RGBDisk 2

