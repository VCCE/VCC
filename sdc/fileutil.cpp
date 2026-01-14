//define USE_LOGGING
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

#include <string>
#include <filesystem>
#include <Shlwapi.h>
#include <vcc/core/logger.h>
#include "fileutil.h"

//----------------------------------------------------------------------
// Get most recent windows error text
//----------------------------------------------------------------------
const char * LastErrorTxt() {
    static char msg[200];
    DWORD error_code = GetLastError();
    FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                    nullptr, error_code,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    msg, sizeof(msg), nullptr );
    return msg;
}
std::string LastErrorString() {
    return LastErrorTxt();
}

//----------------------------------------------------------------------
// Return copy of string with spaces trimmed from end of a string
//----------------------------------------------------------------------
std::string trim_right_spaces(const std::string& s)
{
    size_t end = s.find_last_not_of(' ');
    if (end == std::string::npos)
        return {};
    return s.substr(0, end + 1);
}

//----------------------------------------------------------------------
// Return slash normalized directory part of a path
//----------------------------------------------------------------------
std::string GetDirectoryPart(const std::string& input)
{
    std::filesystem::path p(input);
    std::string out = p.parent_path().string();
    FixDirSlashes(out);
    return out;
}

//----------------------------------------------------------------------
// Return filename part of a path
//----------------------------------------------------------------------
std::string GetFileNamePart(const std::string& input)
{
    std::filesystem::path p(input);
    return p.filename().string();
}

//----------------------------------------------------------------------
// Determine if path is a direcory
//----------------------------------------------------------------------
bool IsDirectory(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && !ec;
}

//-------------------------------------------------------------------
// Convert path directory backslashes to forward slashes
//-------------------------------------------------------------------
void FixDirSlashes(std::string &dir)
{
    if (dir.empty()) return;
    std::replace(dir.begin(), dir.end(), '\\', '/');
    if (dir.back() == '/') dir.pop_back();
}
void FixDirSlashes(char *dir)
{
    if (!dir) return;
    std::string tmp(dir);
    FixDirSlashes(tmp);
    strcpy(dir, tmp.c_str());
}

//-------------------------------------------------------------------
// Copy string to fixed size char array (non terminated)
//-------------------------------------------------------------------
template <size_t N>
void copy_to_fixed_char(char (&dest)[N], const std::string& src)
{
    size_t i = 0;
    for (; i < src.size() && i < N; ++i)
        dest[i] = src[i];
    for (; i < N; ++i)
        dest[i] = ' ';
}

//-------------------------------------------------------------------
// Convert LFN to FAT filename parts, 8 char name, 3 char ext
// A LNF file is less than 4GB and has a short (8.3) name.
//-------------------------------------------------------------------
void sfn_from_lfn(char (&name)[8], char (&ext)[3], const std::string& lfn)
{
    // Special case: parent directory
    if (lfn == "..") {
        copy_to_fixed_char(name, "..");
        std::fill(ext, ext + 3, ' ');
        return;
    }

    size_t dot = lfn.find('.');
    std::string base, extension;

    if (dot == std::string::npos) {
        base = lfn;
    } else {
        base = lfn.substr(0, dot);
        extension = lfn.substr(dot + 1);
    }

    copy_to_fixed_char(name, base);
    copy_to_fixed_char(ext, extension);
}

//-------------------------------------------------------------------
// Convert FAT filename parts to LFN. Returns empty string if invalid
//-------------------------------------------------------------------
std::string lfn_from_sfn(const char (&name)[8], const char (&ext)[3])
{
    std::string base(name, 8);
    std::string extension(ext, 3);

    base = trim_right_spaces(base);
    extension = trim_right_spaces(extension);

    if (base == ".." && extension.empty())
    return "..";

    std::string lfn = base;

    if (!extension.empty())
    lfn += "." + extension;

    if (lfn.empty())
        return {};

    if (!PathIsLFNFileSpecA(lfn.c_str()))
        return {};

    return lfn;
}

//-------------------------------------------------------------------
// Convert string containing possible FAT name and extension to an
// LFN string. Returns empty string if invalid LFN
//-------------------------------------------------------------------
std::string NormalizeInputToLFN(const std::string& s)
{
    if (s.empty()) return {};
    if (s.size() > 11) return {};
    if (s == "..") return "..";

    // LFN candidate
    if (s.find('.') != std::string::npos) {
        if (!PathIsLFNFileSpecA(s.c_str()))
            return {}; // invalid
        return s;
    }

    // SFN candidate
    char name[8];
    char ext[3];
    sfn_from_lfn(name,ext,s);
    return lfn_from_sfn(name,ext);
}

//----------------------------------------------------------------------
// A file path may use 11 char FAT format which does not use a separater
// between name and extension. User is free to use standard dot format.
//    "FOODIR/FOO.DSK"     = FOODIR/FOO.DSK
//    "FOODIR/FOO     DSK" = FOODIR/FOO.DSK
//    "FOODIR/ALONGFOODSK" = FOODIR/ALONGFOO.DSK
//----------------------------------------------------------------------
std::string FixFATPath(const std::string& sdcpath)
{
    std::filesystem::path p(sdcpath);

    auto chop = [](std::string s) {
        size_t space = s.find(' ');
        if (space != std::string::npos) s.erase(space);
        return s;
    };

    std::string fname = p.filename().string();
    if (fname.length() == 11 && fname.find('.') == std::string::npos) {
        auto nam = chop(fname.substr(0,8));
        auto ext = chop(fname.substr(8,3));
        fname = ext.empty() ? nam : nam + "." + ext;
    }

    std::filesystem::path out = p.parent_path() / fname;

    DLOG_C("FixFATPath in %s out %s\n",sdcpath.c_str(),out.generic_string().c_str());
    return out.generic_string();
}

void FixFATPath(char* path, const char* sdcpath)
{
    std::string fixed = FixFATPath(std::string(sdcpath));
    strncpy(path, fixed.c_str(), MAX_PATH);
    path[MAX_PATH - 1] = '\0';
}

