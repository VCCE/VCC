#pragma once

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
//  2025/04/10 - Craig Allsop - Add OpenGL rendering.
//

#include "IDisplay.h"
#include "defines.h"

#if USE_DIRECTX

namespace VCC
{
    struct DirectX : public IDisplayDirectX
    {
        enum
        {
            ERR_UNSUPPORTED = 100,          // unsupported command
            ERR_BADOPTION,                  // invalid option being set
            ERR_UNKNOWN
        };

		explicit DirectX(ISystemState* state);

        int Setup(void* hwnd, int width, int height, int statusHeight, bool fullscreen) override;
        int Render() override;
        int Present() override;
        int Cleanup() override;
        int SetOption(int flagOption, bool enabled) override;
        int GetSurface(Pixel** pixels) override;
        int GetRect(int rectOption, Rect* rect) override;
        int LockSurface() override;
        int UnlockSurface() override;
        void DebugDrawLine(float x1, float y1, float x2, float y2, Pixel color) override {};
        void DebugDrawBox(float x, float y, float w, float h, Pixel color) override {};
        int RenderSignalLostMessage() override;
        int RenderStatusLine(char* statusText) override;

    private:

        ISystemState* state;

        void GetDisplayArea(Rect* rect);
        void GetSurfaceArea(Rect* rect);
        void CheckSurfaces();
    };
}

#endif // USE_DIRECTX


