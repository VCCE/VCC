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

#if USE_OPENGL

#include <vector>

namespace VCC
{
	struct OpenGLFontBox
	{
		float l, t, r, b;
	};

	struct OpenGLFontGlyph
	{
		float advance;					// amount to advance (em)
		OpenGLFontBox dimensions;		// box to draw relative to baseline (em)
		OpenGLFontBox atlasDimensions;	// atlas box (pixels)
	};

	// handle for fonts
	struct OpenGLFont
	{
		int start;						// first character (incl) e.g. 32
		int end;						// last character (excl) e.g. 128
		uint32_t texture;				// open gl texture handle
		float textureWidth;				// texture width
		float textureHeight;			// texture height
		const OpenGLFontGlyph* glyphs;	// array of glyphs
	};
}

#endif // USE_OPENGL
