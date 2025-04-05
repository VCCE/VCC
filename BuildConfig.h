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

///////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Build Config - conditional compile options
//
///////////////////////////////////////////////////////////////////////////////////////////////////


//
// Enable the use of OpenGL display driver, otherwise use DirectX.
//
#ifndef _LEGACY_VCC
#ifndef USE_OPENGL
#define USE_OPENGL true
#endif
#endif

//
// Enable black fill to preserve aspect ratio, otherwise replicate border color.
//
#ifndef USE_BLACKEDGES
#define USE_BLACKEDGES true
#endif

//
// Enable rendering of debug lines
// Needs: USE_OPENGL
//
#ifndef USE_DEBUG_LINES
#define USE_DEBUG_LINES true	
#endif

//
// Enable debug of joystick ramp.
// Needs: USE_DEBUG_LINES
//
#ifndef USE_DEBUG_RAMP
#define USE_DEBUG_RAMP false
#endif

//
// Enable debugging of mouse cursor.
// Needs: USE_DEBUG_LINES 
//
#ifndef USE_DEBUG_MOUSE
#define USE_DEBUG_MOUSE false
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Edit options above rather than these:
//
#if !USE_OPENGL
#define USE_DIRECTX true
#endif

#if USE_OPENGL && USE_DIRECTX
#error Enable either USE_OPENGL or USE_DIRECTX not both.
#endif
