//  Copyright 2015 by Joseph Forgion
//  This file is part of VCC (Virtual Color Computer).
//
//  VCC (Virtual Color Computer) is free software: you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  VCC (Virtual Color Computer) is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License along
//  with VCC. If not, see <http://www.gnu.org/licenses/>.
//

#include "Screenshot.h"
#include <time.h>
#include <sys/stat.h>
#include <cstdint>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "3rdparty/stb/stb_image_write.h"

using namespace VCC;

namespace Screenshot
{
    typedef std::uint8_t uint8_t;

    int Snap(IDisplay* display)
    {
        // get user directory
        const char* userProfile = getenv("USERPROFILE");
        if (!userProfile) return ERR_NOUSERPROFILE;

        // create filename
        char path[_MAX_PATH];
        time_t now = time(NULL);
        auto tm = *localtime(&now);
        auto len = snprintf(path, _MAX_PATH, "%s\\Pictures\\vcc-%04d%02d%02d-%02d%02d%02d-0.png", userProfile, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
        if (len < 0 || len >= _MAX_PATH) return ERR_PATHTOOLONG;

        // check filename clash
        char* next = path + len - 5; // '0'
        struct stat st;
        while (stat(path, &st) == 0)
        {
            if (*next == '9') return ERR_FILECLASH;
            ++(*next);
        }

        if (display->LockSurface() != IDisplay::OK) return ERR_NOLOCK;

        auto cleanup = [&](int code) -> int
        {
            if (display->UnlockSurface() != IDisplay::OK) return ERR_NOUNLOCK;
            return code;
        };

        // get surface
        Pixel* pixels;
        if (display->GetSurface(&pixels) != IDisplay::OK) return cleanup(ERR_NOSURFACE);

        // get dimensions of display surface
        IDisplay::Rect rect;
        if (display->GetRect(IDisplay::OPT_RECT_SURFACE, &rect) != IDisplay::OK) return cleanup(ERR_NOAREA);

        auto w = (int)rect.w;
        auto h = (int)rect.h;

        // convert argb to rgb
        uint8_t* rgb = new uint8_t[w * h * 3];
        if (!rgb) return cleanup(1);

        auto cleanupMem = [&](int code) { delete[] rgb; return cleanup(code); };

        uint8_t* p = rgb;
        for (int i = 0; i < w * h; ++i, ++pixels)
        {
            *(p++) = pixels->r;
            *(p++) = pixels->g;
            *(p++) = pixels->b;
        }

        // write the png
        if (stbi_write_png(path, w, h, 3, rgb, w * 3) == 0)
            return cleanupMem(ERR_WRITEPNG);

        // cleanup
        return cleanupMem(OK);
    }
}