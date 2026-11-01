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

#include "Base64.h"

namespace VCC
{
    namespace
    {
        static const char gEncodeTable[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    }

    ////////////////////////////////////////////////////////////////////////////

    std::string Base64Encode(const uint8_t* data, size_t len)
    {
        std::string out;
        out.reserve(((len + 2) / 3) * 4);

        for (size_t i = 0; i < len; i += 3) {
            uint32_t n = (uint32_t)data[i] << 16;
            if (i + 1 < len) n |= (uint32_t)data[i + 1] << 8;
            if (i + 2 < len) n |= (uint32_t)data[i + 2];

            out += gEncodeTable[(n >> 18) & 0x3F];
            out += gEncodeTable[(n >> 12) & 0x3F];
            out += (i + 1 < len) ? gEncodeTable[(n >> 6) & 0x3F] : '=';
            out += (i + 2 < len) ? gEncodeTable[n & 0x3F] : '=';
        }
        return out;
    }

    ////////////////////////////////////////////////////////////////////////////

    std::vector<uint8_t> Base64Decode(const std::string& s)
    {
        std::vector<uint8_t> out;
        out.reserve((s.size() / 4) * 3);

        uint32_t buf = 0;
        int bits = 0;

        for (char c : s) {
            if (c == '=' || c == '\r' || c == '\n' || c == ' ') continue;
            const char* p = std::strchr(gEncodeTable, c);
            if (!p) continue;
            buf = (buf << 6) | (uint32_t)(p - gEncodeTable);
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                out.push_back((buf >> bits) & 0xFF);
            }
        }
        return out;
    }
}