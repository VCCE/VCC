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
#include "OpenGL.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "3rdparty/stb/stb_image_write.h"

void Screenshot(VCC::IDisplay* display)
{
    // get user directory
    const char *userProfile = getenv("USERPROFILE");
    if (!userProfile) return;

    // create filename
    char path[_MAX_PATH];
    time_t now = time(NULL);
    auto tm = *localtime(&now);
    auto len = snprintf(path, _MAX_PATH, "%s\\Pictures\\vcc-%04d%02d%02d-%02d%02d%02d-0.png", userProfile, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    if (len < 0 || len >= _MAX_PATH) return;

    // check filename clash
    char* next = path + len - 5; // '0'
    struct stat st;
    while (stat(path, &st) == 0)
    {
        if (*next == '9') return;
        ++(*next);
    }

    // get surface
    VCC::Pixel* pixels;
    if (display->GetSurface(&pixels) != VCC::OpenGL::OK) return;

    // get dimensions of display surface
    VCC::OpenGL::Rect rect;
    if (display->GetRect(VCC::OpenGL::OPT_RECT_SURFACE, &rect) != VCC::OpenGL::OK) return;

    auto w = (int)rect.w;
    auto h = (int)rect.h;

    // convert argb to rgb
    uint8_t* rgb = new uint8_t[w*h*3];
    if (!rgb) return;
    uint8_t* p = rgb;
    for (int i = 0; i < w * h; ++i, ++pixels)
    {
        *(p++) = pixels->r;
        *(p++) = pixels->g;
        *(p++) = pixels->b;
    }

    // write the png
    stbi_write_png(path, w, h, 3, rgb, w*3);

    // cleanup
    delete[] rgb;
}

