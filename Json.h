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
#include <variant>
#include <vector>
#include <string>
#include <map>

namespace VCC
{
    struct Json
    {
        using String = std::string;
        using StringView = std::string_view;
        template <class T> using Vector = std::vector<T>;
        template <class K, class T> using Map = std::map<K, T>;
        template <typename ...T> struct Overloads : T... { using T::operator()...; };
        template <typename ...T> Overloads(T...) -> Overloads<T...>;
        using Object = Map<String, Json>;
        using Array = Vector<Json>;
        struct Nil {};
        static Json Empty;
        static Array EmptyArray;
        static Object EmptyObject;
        
        enum
        {
            OK = 0,
            ERR_MISSING_TOKEN,
            ERR_UNEXPECTED_END,
            ERR_UNEXPECTED_ESC,
            ERR_UNKNOWN_TOKEN,
            ERR_UNKNOWN_NUMBER,
        };

        Json() : v(Nil()) {};
        Json(bool b) : v(b) {}
        Json(int n) : v((double)n) {}
        Json(long n) : v((double)n) {}
        Json(double n) : v(n) {}
        Json(const char* s) : v(s) {}
        Json(String s) : v(s) {}
        Json(Array a) : v(a) {}
        Json(Object o) : v(o) {}

        // parse text
        static Json Parse(const String& text);

        // as type accessors
        double AsDouble(double def = 0.0) const;
        int AsInt(int def = 0) const;
        const String& AsString(const String &def = "") const;
        bool AsBool(bool def = false) const;

        // object
        bool IsObject() const;
        Json& operator[](const String& key); // creates!
        const Json& operator[](const String& key) const;
        const Json& At(const String& key) const;
        bool Has(const String& key) const;
        const Object& AsObject() const;
        Object& AsObject(); // creates!

        // array
        bool IsArray() const;
        const Json& operator[](size_t index) const;
        const Json& At(size_t index) const;
        const Array& AsArray() const;
        Array& AsArray(); // creates

        // object/array
        size_t Size() const;

        // if == Empty, everything else is not.
        bool IsEmpty() const;

        // stringify
        String ToString(int opt = 0) const;
        void ToString(std::stringstream& out, int opt = 0, int depth = 0) const;

#if USE_DEBUG_TESTS
        static bool Test();
#endif

        enum Options
        {
            TS_NEWLINES = 1,
            TS_INDENT,
        };

    private:

        // union of the 6 standard json types
        std::variant<Nil, bool, double, String, Array, Object> v;

    };
}