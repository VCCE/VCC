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

    Debug Server DAP - Craig Allsop 2026
*/

#pragma once

#include "BuildConfig.h"
#include <vector>
#include <string>
#include <string_view>

namespace VCC
{
    // parse integer from string view
    int ParseInt(std::string_view sv);
    double ParseDouble(std::string_view sv);

    // trim white space
    std::string_view TrimWs(const std::string_view &sv);

    // load entire file and return array of lines
    std::vector<std::string> LoadTextFile(const std::string& path);

    // split path into pieces (todo: use path_view or path)
    std::vector<std::string> SplitPath(const std::string& path);

    // join path elements (todo: use path_view or path)
    std::string JoinPath(const std::string& base, const std::vector<std::string>& path);
    std::string JoinPath(const std::vector<std::string>& path);

    // conversion to lower case
    std::string ToLower(const std::string& str);
}
