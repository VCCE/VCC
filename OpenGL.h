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

#if USE_OPENGL

#include "OpenGLFont.h"

namespace VCC
{
	struct OpenGL : public IDisplay
	{
		enum // result codes
		{
			OK,						// no error
			ERR_INITIALIZED,		// already initialized
			ERR_NOTINITIALIZED,		// not initialized
			ERR_TMPREGISTERCLASS,	// unable to register temporary class
			ERR_TMPCREATEWINDOW,	// unable to create temporary window
			ERR_TMPGETDC,			// unable to get temporary window device context
			ERR_REGISTERCLASS,		// unable to register gl class
			ERR_CREATEWINDOW,		// unable to create gl window
			ERR_GETDC,				// unable to get gl window device context
			ERR_CREATECONTEXT,		// unable to create open gl context
			ERR_SETPIXELFORMAT,		// unable to configure pixel format
			ERR_MISSINGAPIS,		// unable to get open gl apis
			ERR_GLVERSION,			// unable to get open gl version
			ERR_BADOPTION,			// invalid option being set
			ERR_FONTHBITMAP,		// unable to load font bitmap
			ERR_FONTDC,				// unable to create device context for font
			ERR_FONTGETDIBITS,		// unable to get font bitmap bits
			ERR_FONTTEXIMAGE2D,		// unable to upload font to texture
		};

		enum // flagOption
		{
			OPT_FLAG_ASPECT = 1,	// force aspect ratio (default) ELSE stretch display
			OPT_FLAG_NTSC,			// use 50hz aspect ELSE 60hz aspect (default)
		};

		enum // rectOption
		{
			OPT_RECT_DISPLAY = 1,	// display surface rectangle
			OPT_RECT_RENDER,		// entire render output rectangle
		};

		OpenGL() : detail(nullptr) {}

		int Setup(void* hwnd, int width, int height, int statusHeight) override;
		int Render() override;
		int RenderBox(float x, float y, float w, float h, Pixel color, bool filled) override;
		int RenderText(const OpenGLFont* font, float x, float y, float size, const char* text) override;
		int Present() override;
		int Cleanup() override;
		
		int SetOption(int flagOption, bool enabled) override;
		int GetSurface(Pixel** pixels) override;
		int GetRect(int rectOption, Rect* area) override;
		int LoadFont(const OpenGLFont** outFont, int bitmapRes, const OpenGLFontGlyph* glyphs, int start, int end) override;

		void DebugDrawLine(float x1, float y1, float x2, float y2, Pixel color) override;
		void DebugDrawBox(float x, float y, float w, float h, Pixel color) override;

	private:
		struct Detail;
		Detail* detail;
	};
}

#endif // USE_OPENGL
