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

#include "StringUtils.h"

#include <charconv>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace VCC
{
    int ParseInt(std::string_view sv)
    {
        int value;
        auto end = sv.data() + sv.size();
        auto [p, e] = std::from_chars(sv.data(), end, value);
        if (e != std::errc{}) throw std::invalid_argument("stoi");
        if (p != end) throw std::invalid_argument("trailing chars");
        return value;
    }

    //////////////////////////////////////////////////////////////////////////

    double ParseDouble(std::string_view sv)
    {
        double value;
        auto end = sv.data() + sv.size();
        auto [p, e] = std::from_chars(sv.data(), end, value);
        if (e != std::errc{}) throw std::invalid_argument("stod");
        if (p != end) throw std::invalid_argument("trailing chars");
        return value;
    }

    //////////////////////////////////////////////////////////////////////////

    std::string_view TrimWs(const std::string_view& sv)
    {
        auto ws = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
        auto copy = sv;

        while (!copy.empty() && ws(copy.front()))
            copy.remove_prefix(1);

        while (!copy.empty() && ws(copy.back()))
            copy.remove_suffix(1);

        return copy;
    }

    //////////////////////////////////////////////////////////////////////////

    std::vector<std::string> LoadTextFile(const std::string& path)
    {
        std::ifstream file(path);
        if (!file) return {};

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) 
        {
            if (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                line.pop_back();
            lines.push_back(std::move(line));
        }
        return lines;
    }

    //////////////////////////////////////////////////////////////////////////

    std::string ToLower(const std::string& str)
    {
        std::string lower(str);
        for (size_t i = 0; i < str.size(); ++i)
            lower[i] = (char)std::tolower(lower[i]);
        return lower;
    }

    //////////////////////////////////////////////////////////////////////////

    std::vector<std::string> SplitPath(const std::string& path)
    {
        std::vector<std::string> names;
        std::string_view sv(path);
        while (!sv.empty())
        {
            auto slash = sv.find_first_of("/\\");
            if (slash == std::string_view::npos)
            {
                names.emplace_back(sv);
                return names;
            }
            if (slash > 0)
            {
                auto name = std::string(sv.substr(0, slash));
                if (name.back() == ':')
                    names.emplace_back(ToLower(name));
                else                    
                    names.emplace_back(name);
            }
            sv.remove_prefix(slash + 1);
        }
        return names;
    }

    //////////////////////////////////////////////////////////////////////////

    std::string JoinPath(const std::string& base, const std::vector<std::string>& path)
    {
        std::string result(base);
        for (size_t i = 0; i < path.size(); ++i)
        {
            result += '/';
            result += path[i];
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////////

    std::string JoinPath(const std::vector<std::string>& path)
    {
        std::string result;
        for (size_t i = 0; i < path.size(); ++i)
        {
            if (i) result += '/';
            result += path[i];
        }
        return result;
    }

}