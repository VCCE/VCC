//======================================================================
// General purpose Host file utilities.  EJ Jaquay 2026
//
// This file is part of VCC (Virtual Color Computer).
// Vcc is Copyright 2015 by Joseph Forgione
//
// VCC (Virtual Color Computer) is free software, you can redistribute
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of
// the License, or (at your option) any later version.
//
// VCC (Virtual Color Computer) is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with VCC (Virtual Color Computer).  If not, see
// <http://www.gnu.org/licenses/>.
//
//======================================================================
#pragma once

#include <string>
#include <filesystem>
#include <Shlwapi.h>

//=========================================================================
// Host file utilities.  Most of these are general purpose
//=========================================================================

// Get most recent windows error text
const char * LastErrorTxt();
std::string LastErrorString();

// Convert backslashes to slashes in directory string / char
void FixDirSlashes(std::string &dir);
void FixDirSlashes(char *dir);

// copy string into fixed-size char array and blank-pad
template <size_t N>
void copy_to_fixed_char(char (&dest)[N], const std::string& src);

// Return copy of string with spaces trimmed from end of a string
std::string trim_right_spaces(const std::string &s);

// Convert LFN to FAT filename parts, 8 char name, 3 char ext
void sfn_from_lfn(char (&name)[8], char (&ext)[3], const std::string& lfn);

// Convert FAT file name parts to LFN. Returns empty string if invalid LFN
std::string lfn_from_sfn(const char (&name)[8], const char (&ext)[3]);

// Return slash normalized directory part of a path
std::string GetDirectoryPart(const std::string& input);

// Return filename part of a path
std::string GetFileNamePart(const std::string& input);

// SDC interface often presents a path which does not use a dot to 
// delimit name and extension: "FOODIR/FOO     DSK" -> FOODIR/FOO.DSK
std::string FixFATPath(const std::string& sdcpath);
void FixFATPath(char* path, const char* sdcpath);

// Determine if path is a direcory
bool IsDirectory(const std::string& path);

