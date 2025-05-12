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

#include "BuildConfig.h"
#include <cstdint>

namespace VCC
{
	typedef std::uint32_t uint32_t;
	typedef std::uint8_t uint8_t;

	//
	// A single pixel 32bit, alpha is unused
	//
	union Pixel
	{
		uint32_t pixel; // ARGB
		struct
		{
			uint8_t b;
			uint8_t g;
			uint8_t r;
			uint8_t a;
		};

		Pixel() {}
		Pixel(uint32_t p) : pixel(p) {}
		Pixel(uint8_t r, uint8_t g, uint8_t b) : a(255), r(r), g(g), b(b) {}
		bool operator==(const Pixel& o) const { return o.pixel == pixel; }
		bool operator!=(const Pixel& o) const { return !operator==(o); }
	};

	//
	// Just standard colors
	//
	static Pixel ColorWhite(255, 255, 255);
	static Pixel ColorRed(255, 0, 0);
	static Pixel ColorGreen(0, 255, 0);
	static Pixel ColorBlue(0, 0, 255);
	static Pixel ColorYellow(255, 255, 0);
	static Pixel ColorBlack(0, 0, 0);

	//
	// Interface for display output
	//
	// return values:
	//		0 = no error
	//		n = see ERR_* errors in implemenation
	//
	struct IDisplay
	{
		enum
		{
			OK,						// no error
		};

		struct Rect
		{
			float x, y, w, h;
		};

		enum // rectOption
		{
			OPT_RECT_DISPLAY = 1,	// display surface rectangle
			OPT_RECT_RENDER,		// entire render output rectangle
			OPT_RECT_SURFACE,		// get rectangle of surface
		};

		enum // flagOption
		{
			OPT_FLAG_ASPECT = 1,	// force aspect ratio (default) ELSE stretch display
			OPT_FLAG_NTSC,			// use 50hz aspect ELSE 60hz aspect (default)
			OPT_FLAG_RESIZEABLE,
		};

		// setup display:
		//	width = full window width
		//	height = full window height not including status bar
		//  statusHeight = height to leave for status bar
		virtual int Setup(void* hwnd, int width, int height, int statusHeight, bool fullscreen) = 0;

		//
		// render the surface to the screen
		//
		virtual int Render() = 0;

		//
		// present buffer to the screen
		//
		virtual int Present() = 0;

		//
		// undo entire setup and close display
		//
		virtual int Cleanup() = 0;

		//
		// configure boolean option
		//
		virtual int SetOption(int flagOption, bool enabled) = 0;

		//
		// return the virtual surface (RGBA format) to draw into
		//
		virtual int GetSurface(Pixel** pixels) = 0;

		//
		// returns the queried rectangle
		//
		virtual int GetRect(int rectOption, Rect* area) = 0;

		//
		// draw line/box in surface coordinates
		//
		virtual void DebugDrawLine(float x1, float y1, float x2, float y2, Pixel color) = 0;
		virtual void DebugDrawBox(float x, float y, float w, float h, Pixel color) = 0;

		//
		// for access to GetSurface
		//
		virtual int LockSurface() = 0;
		virtual int UnlockSurface() = 0;
	};

	struct OpenGLFont;
	struct OpenGLFontGlyph;

	struct IDisplayOpenGL : IDisplay
	{
		//
		// render a box on the screen
		//
		virtual int RenderBox(float x, float y, float w, float h, Pixel color, bool filled) = 0;

		//
		// render the text on screen for full screen
		//
		virtual int RenderText(const OpenGLFont* font, float x, float y, float size, const char* text) = 0;

		//
		// create a new font from bitmapRes window resource
		// out: outFont or null
		//
		virtual int LoadFont(const OpenGLFont** outFont, int bitmapRes, const OpenGLFontGlyph* glyphs, int start, int end) = 0;

		virtual int UnlockSurface() { return OK; }
	};

	struct IDisplayDirectX : IDisplay
	{
		//
		// render the signal lost message for directx
		//
		virtual int RenderSignalLostMessage() = 0;

		//
		// render the status line on screen for directx
		//
		virtual int RenderStatusLine(char* statusText) = 0;
	};
}
