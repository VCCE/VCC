#ifndef __DEFINES_H__
#define __DEFINES_H__
/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/
//Speed throttling
constexpr auto FRAMEINTERVAL = 120u;	//Number of frames to sum the framecounter over
constexpr auto TARGETFRAMERATE = 60u;	//Number of throttled Frames per second to render

//CPU 
constexpr auto _894KHZ = 57u;
constexpr auto JIFFIESPERLINE = _894KHZ * 4;
constexpr auto LINESPERFIELD = 262u;

// Audio handling
constexpr auto BITRATE = LINESPERFIELD * TARGETFRAMERATE;
constexpr auto BLOCK_SIZE = LINESPERFIELD * 2;
constexpr auto BLOCK_COUNT = 6u;

//Misc
constexpr auto QUERY = 255u;
constexpr auto INDEXTIME = LINESPERFIELD * TARGETFRAMERATE / 5;

#endif

