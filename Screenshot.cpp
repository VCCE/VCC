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

    int SnapSurface(IDisplay* display, Option opt, uint8_t **rgb, int *width, int *height)
    {
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

        if (width) *width = w;
        if (height) *height = h;

        // convert argb to rgb
        int length = w * h * (opt == OPT_RGB) ? 3 : 4;
        *rgb = new uint8_t[length];
        if (!*rgb) return cleanup(1);

        uint8_t* p = *rgb;
        for (int i = 0; i < w * h; ++i, ++pixels)
        {
            *(p++) = pixels->r;
            *(p++) = pixels->g;
            *(p++) = pixels->b;
            if (opt == OPT_RGBA)
                *(p++) = 255;
        }
        return cleanup(OK);
    }

    int Snap(IDisplay* display, const char* fname)
    {
		char path[_MAX_PATH];

		if (!fname)
		{
			// get user directory
			const char* userProfile = getenv("USERPROFILE");
			if (!userProfile) return ERR_NOUSERPROFILE;

			// create filename
			time_t now = time(nullptr);
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
			fname = path;
		}

        uint8_t* rgb;
        int w;
        int h;

        int result = SnapSurface(display, OPT_RGB, &rgb, &w, &h);
        auto cleanupMem = [&](int code) { delete[] rgb; return code; };
        if (result != OK) return cleanupMem(result);

        // write the png
        if (stbi_write_png(fname, w, h, 3, rgb, w * 3) == 0)
            return cleanupMem(ERR_WRITEPNG);

        // cleanup
        return cleanupMem(OK);
    }
}