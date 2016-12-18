#pragma once

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

#include "xTypes.h"

//
// Windows incudes
//
//#define NO_WARN_MBCS_MFC_DEPRECATION
//#define _WIN32_WINNT 0x05010000 // I want to support XP
//#define STRICT
//#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
//#ifndef ABOVE_NORMAL_PRIORITY_CLASS 
//#define ABOVE_NORMAL_PRIORITY_CLASS  32768
//#endif 

#define DIRECTINPUT_VERSION 0x0800
#define _WINSOCKAPI_				// stops windows.h including winsock.h
#define WIN32_LEAN_AND_MEAN			// Exclude rarely-used stuff from Windows headers

#include <windows.h>
#include <windowsx.h>

#include <stdint.h>

//Speed throttling
constexpr uint_t FRAMEINTERVAL = 120;	//Number of frames to sum the framecounter over
#define TARGETFRAMERATE 60	//Number of throttled Frames per second to render

//CPU 
constexpr double FRAMESPERSECORD = 59.923;	//The coco really runs at about 59.923 Frames per second
constexpr double LINESPERSCREEN  = 262;
constexpr double PICOSECOND      = 1000000000;
constexpr double COLORBURST      = 3579545; 
constexpr int    AUDIOBUFFERS    = 12;

//Misc
constexpr uint8_t QUERY = 255;
#define INDEXTIME ((LINESPERSCREEN * TARGETFRAMERATE)/5)


//Common CPU defs
#define IRQ		1
#define FIRQ	2
#define NMI		3

// Common RAM sizes
enum MemorySizes: uint8_t {
    MEM_128K = 0,
    MEM_512K,
    MEM_1M,
    MEM_2M
};



//
// CPU plug-in API
//
extern void (*CPUInit)(void);
extern int  (*CPUExec)( int);
extern void (*CPUReset)(void);
extern void (*CPUAssertInterupt)(unsigned char,unsigned char);
extern void (*CPUDeAssertInterupt)(unsigned char);
extern void (*CPUForcePC)(unsigned short);


struct SystemState
{
    HWND			WindowHandle;
    HWND			ConfigDialog;
    HINSTANCE		WindowInstance;
    unsigned char	*RamBuffer;
    unsigned short	*WRamBuffer;
    unsigned char	RamSize;
    double			CPUCurrentSpeed;
    unsigned char	DoubleSpeedMultiplyer;
    unsigned char	DoubleSpeedFlag;
    unsigned char	TurboSpeedFlag;
    unsigned char	CpuType;

    int				FrameSkip;
    int				BitDepth;
    unsigned char	*PTRsurface8;
    unsigned short	*PTRsurface16;
    unsigned int	*PTRsurface32;
    long			SurfacePitch;
    int				LineCounter;
    int				ScanLines;
    //bool			InRender;
    //unsigned char	PauseEmuLoop;
    //unsigned char	Waiting;
    unsigned char	FullScreen;

    int				EmulationRunning;
    int				ResetPending;
    POINT			WindowSize;
    char			StatusLine[256];
};
