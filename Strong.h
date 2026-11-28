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

#include <map>
#include <unordered_map>
#include <cstddef>

namespace VCC
{
    //
    // explicit typing for integer/bool
    //
    template <typename T, typename Tag>
    struct Strong
    {
        using Type = T;
        constexpr Strong() = default;
        explicit constexpr Strong(T v) : v(v) {}
        constexpr operator T() const { return v; }
        constexpr bool operator<(const Strong& o) const { return v < o.v; }
        constexpr bool operator==(const Strong& o) const { return v == o.v; }

        template <typename T2, typename Tag2>
        friend bool operator==(const Strong& a, const Strong<T2, Tag2>& b) {
            static_assert(std::is_same_v<Tag, Tag2>,
                "Cannot compare different Strong types");
            return a.v == b.v;
        }

        template <typename T2, typename Tag2>
        friend bool operator!=(const Strong& a, const Strong<T2, Tag2>& b) {
            static_assert(std::is_same_v<Tag, Tag2>,
                "Cannot compare different Strong types");
            return a.v != b.v;
        }
    private:
        T v{};
        friend struct std::hash<VCC::Strong<T, Tag>>;
    };
}

template <typename T, typename Tag>
struct std::hash<VCC::Strong<T, Tag>> 
{
    size_t operator()(const VCC::Strong<T, Tag>& s) const { return std::hash<T>{}(s.v); }
};
