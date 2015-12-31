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

#include <cstdint>

//Speed throttling
#define FRAMEINTERVAL 120	//Number of frames to sum the framecounter over
#define TARGETFRAMERATE 60	//Number of throttled Frames per second to render
#define SAMPLESPERFRAME 262



//CPU 
#define FRAMESPERSECORD (double)59.923	//The coco really runs at about 59.923 Frames per second
#define LINESPERSCREEN (double)262
#define PICOSECOND (double)1000000000
#define COLORBURST (double)3579545 
#define AUDIOBUFFERS 12
//Misc
#define MAX_LOADSTRING 100
#define QUERY 255
#define INDEXTIME ((LINESPERSCREEN * TARGETFRAMERATE)/5)


//Common CPU defs
#define IRQ		1
#define FIRQ	2
#define NMI		3

extern void (*CPUInit)(void);
extern int  (*CPUExec)( int);
extern void (*CPUReset)(void);
extern void (*CPUAssertInterupt)(unsigned char,unsigned char);
extern void (*CPUDeAssertInterupt)(unsigned char);
extern void (*CPUForcePC)(unsigned short);


typedef struct {
	unsigned char ScanCode1;
	unsigned char ScanCode2;
	unsigned char Row1;
	unsigned char Col1;
	unsigned char Row2;
	unsigned char Col2;
} KeyBoardTable;

typedef struct {
	unsigned char UseMouse;
	unsigned char Up;
	unsigned char Down;
	unsigned char Left;
	unsigned char Right;
	unsigned char Fire1;
	unsigned char Fire2;
	unsigned char DiDevice;
	unsigned char HiRes;
} JoyStick;


typedef struct 
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
unsigned char	FrameSkip;
unsigned char	BitDepth;
unsigned char	*PTRsurface8;
unsigned short	*PTRsurface16;
unsigned int	*PTRsurface32;
long			SurfacePitch;
unsigned short	LineCounter;
unsigned char	ScanLines;
//bool			InRender;
//unsigned char	PauseEmuLoop;
//unsigned char	Waiting;
unsigned char	EmulationRunning;
unsigned char	ResetPending;
POINT			WindowSize;
unsigned char	FullScreen;
char			StatusLine[256];
} SystemState;

static char RateList[4][7]={"Mute","11025","22050","44100"};
static unsigned short iRateList[4]={0,11025,22050,44100};
#define TAPEAUDIORATE 44100
