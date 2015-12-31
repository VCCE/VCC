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
#define FRAMEINTERVAL 120	//Number of frames to sum the framecounter over
#define TARGETFRAMERATE 60	//Number of throttled Frames per second to render

// Audio handling
#define BITRATE (LINESPERFIELD*TARGETFRAMERATE)
#define BLOCK_SIZE  LINESPERFIELD*2
#define BLOCK_COUNT 6

//CPU 
#define _894KHZ	57
#define JIFFIESPERLINE  (_894KHZ*4)
#define LINESPERFIELD 262

//Misc
#define MAX_LOADSTRING 100
#define QUERY 255
#define INDEXTIME ((LINESPERFIELD * TARGETFRAMERATE)/5)


//Common CPU defs
#define IRQ		1
#define FIRQ	2
#define NMI		3


#endif
