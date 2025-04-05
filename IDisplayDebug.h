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

//
// NOTE: USE_DEBUG_LINES must be enabled to see debug lines.
//

void DebugDrawBox(float x, float y, float w, float h, VCC::Pixel color);
void DebugDrawLine(float x, float y, float x2, float y2, VCC::Pixel color);

inline void DebugDrawBox(int x, int y, int w, int h, VCC::Pixel color) 
{
	DebugDrawBox((float)x, (float)y, (float)w, (float)h, color);
}

inline void DebugDrawLine(int x1, int y1, int x2, int y2, VCC::Pixel color) 
{ 
	DebugDrawLine((float)x1, (float)y1, (float)x2, (float)y2, color);
}
