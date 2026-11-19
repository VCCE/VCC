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

#include "Json.h"

#include <functional>
#include <assert.h>
#include <stdexcept>
#include <sstream>

#include "StringUtils.h"

namespace VCC
{
    Json Json::Empty{};
    Json::Array Json::EmptyArray{};
    Json::Object Json::EmptyObject{};

    ////////////////////////////////////////////////////////////////////////////

    // accessors
    double Json::AsDouble(double def) const
    {
        return std::holds_alternative<double>(v) ? std::get<double>(v) : def;
    }

    ////////////////////////////////////////////////////////////////////////////

    int Json::AsInt(int def) const
    {
        return (int)AsDouble(def);
    }

    ////////////////////////////////////////////////////////////////////////////

    const Json::String &Json::AsString(const String &def) const
    {
        return std::holds_alternative<String>(v) ? std::get<String>(v) : def;
    }

    ////////////////////////////////////////////////////////////////////////////

    bool Json::AsBool(bool def) const
    {
        return std::holds_alternative<bool>(v) ? std::get<bool>(v) : def;
    }

    ////////////////////////////////////////////////////////////////////////////

    // object
    Json& Json::operator[](const String& key)
    {
        return AsObject()[key];
    }

    const Json& Json::operator[](const String& key) const
    {
        return At(key);
    }

    ////////////////////////////////////////////////////////////////////////////

    const Json& Json::At(const String& key) const
    {
        if (!IsObject()) return Empty;
        auto& o = std::get<Object>(v);
        auto it = o.find(key);
        if (it != o.end()) return Empty;
        return it->second;
    }

    const Json& Json::operator[](size_t index) const
    {
        return At(index);
    }

    ////////////////////////////////////////////////////////////////////////////

    bool Json::Has(const String& key) const
    {
        return IsObject() && AsObject().find(key) != AsObject().end();
    }

    ////////////////////////////////////////////////////////////////////////////

    bool Json::IsObject() const
    {
        return std::holds_alternative<Object>(v);
    }

    ////////////////////////////////////////////////////////////////////////////

    bool Json::IsArray() const
    {
        return std::holds_alternative<Array>(v);
    }

    ////////////////////////////////////////////////////////////////////////////

    const Json::Object& Json::AsObject() const
    {
        return IsObject() ? std::get<Object>(v) : EmptyObject;
    }

    Json::Object& Json::AsObject()
    {
        if (!IsObject()) v = Object{};
        return std::get<Object>(v);
    }

    ////////////////////////////////////////////////////////////////////////////

    const Json::Array& Json::AsArray() const
    {
        return IsArray() ? std::get<Array>(v) : EmptyArray;
    }

    Json::Array& Json::AsArray()
    {
        if (!IsArray()) v = Array{};
        return std::get<Array>(v);
    }

    ////////////////////////////////////////////////////////////////////////////

    const Json& Json::At(size_t index) const
    {
        if (!IsArray()) return Empty;
        auto& a = AsArray();
        if (index >= a.size()) 
            return Empty;
        return a[index];
    }

    ////////////////////////////////////////////////////////////////////////////

    size_t Json::Size() const
    {
        if (IsArray()) return AsArray().size();
        if (IsObject()) return AsObject().size();
        return 0;
    }

    bool Json::IsEmpty() const
    {
        return this == &Empty;
    }

    ////////////////////////////////////////////////////////////////////////////

    Json Json::Parse(const String& text)
    {
        int err = 0;
        StringView sv(text);
        std::function<Json()> pValue;

        auto skip = [&](int n = 1) { sv.remove_prefix(n); };
        auto empty = [&]() { return sv.empty(); };
        auto ch = [&]() { return sv.front(); };
        auto ws = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
        auto digit = [](char c) { return isdigit(c) || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-'; };
        auto skipWs = [&]() { while (!err && !sv.empty() && ws(ch())) skip(); };

        // expect character 'c'
        auto eat = [&](char c)
        {
            if (empty()) err = ERR_UNEXPECTED_END;
            if (ch() != c) err = ERR_MISSING_TOKEN;
            skip();
            skipWs();
        };

        // skip optional character 'c'
        auto eatOpt = [&](char c)
        {
            if (empty()) { err = ERR_UNEXPECTED_END; return; }
            if (ch() == c) skip();
            skipWs();
        };

        auto pString = [&]()
        {
            skip(); // "
            String result;
            while (!err && !sv.empty())
            {
                auto c = ch();
                // end of quote
                if (c == '"')
                {
                    eat('"');
                    return result;
                }
                // escape sequence
                if (c == '\\')
                {
                    skip();
                    c = ch();
                    if (c == '"') result += '"';
                    else if (c == '\\') result += '\\';
                    else if (c == 'n') result += '\n';
                    else if (c == 'r') result += '\r';
                    else if (c == 't') result += '\t';
                    else if (c == 'u') { err = ERR_UNEXPECTED_ESC; /*todo*/ }
                    else result += c;
                }
                else result += c;
                skip();
            }
            return result;
        };

        auto pNumber = [&]()
        {
            size_t i = 0;
            while (!err && i < sv.size() && digit(sv[i]))
                ++i;
            if (i == 0) { err = ERR_UNKNOWN_TOKEN; return 0.0; }
            auto num = ParseDouble(sv.substr(0, i));
            skip(i);
            return num;
        };

        auto pObject = [&]()
        {
            auto obj = Json{ Json::Object{} };
            eat('{');
            while (ch() != '}')
            {
                auto key = pString();
                skipWs();
                eat(':');
                obj[key] = pValue();
                skipWs();
                eatOpt(',');
            }
            eat('}');
            return obj;
        };

        auto pArray = [&]()
        {
            auto arr = Json{ Json::Array{} };
            eat('[');
            while (!empty() && ch() != ']')
            {
                std::get<Json::Array>(arr.v).push_back(pValue());
                skipWs();
                eatOpt(',');
            }
            eat(']');
            return arr;
        };

        pValue = [&]()
        {
            skipWs();
            if (err) return Empty;
            if (sv.empty()) return Empty;
            auto c = ch();
            if (c == '{') return pObject();
            if (c == '[') return pArray();
            if (c == '"') return Json(pString());
            if (c == 't') { skip(4); return Json{ true }; };
            if (c == 'f') { skip(5); return Json{ false }; };
            if (c == 'n') { skip(4); return Empty; }
            return Json(pNumber());
        };

        auto result = pValue();
        if (err) return Empty;
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////

    Json::String Json::ToString(int opt) const
    {
        std::stringstream ss;
        ToString(ss, opt, 0);
        return ss.str();
    }

    ////////////////////////////////////////////////////////////////////////////

    void Json::ToString(std::stringstream& out, int opt, int depth) const
    {
        auto sp = [&](int n) { for (int i = 0; i < n; ++i) out << ' '; };

        auto fromDouble = [&](double d)
        {
            // if an integer then use it
            if (d == (long long)d) 
                out << std::to_string((long long)d);
            else
                out << d;
        };

        // quote string and escape certain characters
        auto fromString = [&](const String& s)
        {
            out << '"';
            for (char c : s)
            {
                switch (c)
                {
                    case '"': out << "\\\""; break;
                    case '\\': out << "\\\\"; break;
                    case '\n': out << "\\n"; break;
                    case '\r': out << "\\r"; break;
                    case '\t': out << "\\t"; break;
                    default:
                        if ((unsigned char)c < 0x20)
                        {
                            char buf[10];
                            std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                            out << buf;
                        }
                        else out << c;
                }
            }
            out << '"';
        };

        auto fromArray = [&](const Array& a)
        {
            out << '[';
            ++depth;
            for (size_t i = 0; i < a.size(); ++i)
            {
                if (i) out << ',';
                if (opt & TS_NEWLINES) out << '\n';
                if (opt & TS_INDENT) sp(depth);
                a[i].ToString(out, opt, depth);
            }
            --depth;
            if (opt & TS_NEWLINES) out << '\n';
            if (opt & TS_INDENT) sp(depth);
            out << ']';
        };

        auto fromObject = [&](const Object& o)
        {
            out << '{';
            ++depth;
            bool first = true;
            for (auto& [k, v] : o)
            {
                if (!first) out << ',';
                if (opt & TS_NEWLINES) out << '\n';
                if (opt & TS_INDENT) sp(depth);
                first = false;
                fromString(k);
                out << ':';
                v.ToString(out, opt, depth);
            }
            --depth;
            if (opt & TS_NEWLINES) out << '\n';
            if (opt & TS_INDENT) sp(depth);
            out << '}';
        };

        std::visit(Overloads{
            [&](Nil n) { out << "null"; },
            [&](bool b) { out << (b ? "true" : "false"); },
            [&](double d) { fromDouble(d); },
            [&](const String& s) { fromString(s); },
            [&](const Array& a) { fromArray(a); },
            [&](const Object& o) { fromObject(o); }
            }, v);
    }

    ////////////////////////////////////////////////////////////////////////////

#if USE_DEBUG_TESTS
    bool Json::Test()
    {
        struct State { Json j; int c = 0; };

        // helpers
        auto c = [](State& s, auto func) { return [&]() { s.c++; func(); }; }; // count calls
        auto f = [](auto ...func) { return [=](State& s) { (func(s), ...); }; }; // run sequence of funcs
        auto t = [](auto check, auto func) { State s; func(s); return check(s); }; // run func & check
        auto alt = [](auto ty, auto v) { return std::holds_alternative<typename decltype(ty)>(v); };
        auto get = [](auto ty, auto v) { return std::get<typename decltype(ty)>(v); };

        // json data
        auto arrayData1 = "[1,2,3,4,5]";
        auto objectData1 = "{ \"e1\":{}, \"key\" : true, \"gime\": 86, \"cpu\" :\"6809\", \"e2\":null} }";

        // operations
        auto jEmpty = [](State& s) {};
        auto jNumber = [](State& s) { s.j = 55.0; };
        auto jString = [](State& s) { s.j = "hello"; };
        auto jObject = [](State& s) { s.j = Object{ {"name","coco"}, {"model",3} }; };
        auto jArray = [](State& s) { s.j = Array{ 1,2,3 }; };
        auto mapAppendKey = [](State& s) { s.j["gime"] = 86; };
        auto pArray = [=](State& s) { s.j = Parse(arrayData1); };
        auto pObject = [=](State& s) { s.j = Parse(objectData1); };

        // evaluations
        auto getIsType = [=](auto ty) { return [=](State& s) { return alt(ty, s.j.v); }; };
        auto getValue = [=](auto v) { return [=](State& s) { return get(v, s.j.v); }; };
        auto getKey = [=](auto k) { return [=](State& s) { return s.j[k]; }; };
        auto getKeyType = [=](auto k, auto ty) { return [=](State& s) { return alt(ty, s.j[k].v); }; };

        // queries
        auto isType = [=](auto ty, auto func) { return t(getIsType(ty), func); };
        auto match = [=](auto m, auto func) { return t(getValue(m), func) == m; };
        auto key = [=](auto k, auto func) { return t(getKey(k), func); };
        auto debug = [=](auto func) { return t(debugStop, func); };
        auto keyType = [=](auto k, auto ty, auto func) { return t(getKeyType(k,ty), func); };

        // test sequences
        auto testAppKey = f(jObject, mapAppendKey);

        // tests
        assert(isType(Nil{}, jEmpty));                      // check empty is created as Nil
        assert(isType(double{}, jNumber));                  // check number is type double
        assert(isType(String{}, jString));                  // check string is type string
        assert(isType(Array{}, jArray));                    // check string is type array
        assert(isType(Object{}, jObject));                  // check string is type object
        assert(match(55.0, jNumber));                       // check number is 55.0
        assert(match(std::string("hello"), jString));       // check string is "hello"
        assert(isType(Object{}, testAppKey));               // check string is type object
        assert(key("gime", testAppKey).AsDouble() == 86.0); // check gime key is equal 86
        assert(isType(Array{}, pArray));                    // check string is type array
        assert(isType(Object{}, pObject));                  // check string is type object
        assert(key("gime", pObject).AsDouble() == 86.0);    // check parsed value expected 86
        assert(keyType("e2", Nil{}, pObject));              // check empty is created as Nil
        assert(keyType("cpu", String{}, pObject));          // check field is string type
        assert(key("cpu", pObject).AsString() == "6809");   // check parsed value is 6809
        return true;
    }

    static bool gTestJson = Json::Test();
#endif
}