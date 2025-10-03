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

#include "defines.h"
#include "IDisplay.h"

namespace VCC
{
	//
	// No open display, just maintains surface.
	//
	struct DisplayNone : IDisplayDirectX, IDisplayOpenGL
	{
		explicit DisplayNone(ISystemState* state) : surface(nullptr), state(state) {}

		int Setup(void* hwnd, int width, int height, int statusHeight, bool fullscreen) override
		{
			area.x = 0;
			area.y = 0;
			area.w = (float)width;
			area.h = (float)height;
			surface = new Pixel[width * height];
			return OK;
		}
		int Cleanup() override
		{
			delete[] surface;
			surface = nullptr;
			return OK;
		}
		int GetSurface(Pixel** pixels) override
		{
			*pixels = surface;
			return OK;
		}
		int GetRect(int rectOption, Rect* rect) override
		{
			if (rect)
			{
				memcpy(rect, &area, sizeof(*rect));
				return OK;
			}
			return !OK;
		}
		int LockSurface() override
		{
			state->SetSurface(surface, 3, 640);
			return OK;
		}

		int Render() override { return OK; }
		int Present() override { return OK; }
		int UnlockSurface() override { return OK; }
		int SetOption(int flagOption, bool enabled) override { return OK; }
		void DebugDrawLine(float x1, float y1, float x2, float y2, Pixel color) override {}
		void DebugDrawBox(float x, float y, float w, float h, Pixel color) override {}
		int RenderSignalLostMessage() override { return OK; }
		int RenderBox(float x, float y, float w, float h, Pixel color, bool filled) override { return OK; }
		int RenderText(const OpenGLFont* font, float x, float y, float size, const char* text) override { return OK; }
		int LoadFont(const OpenGLFont** outFont, int bitmapRes, const OpenGLFontGlyph* glyphs, int start, int end) override { return OK; }
		int RenderStatusLine(char* statusText) override { return OK; }

	private:

		Pixel* surface;
		ISystemState* state;
		Rect area;
	};
}
