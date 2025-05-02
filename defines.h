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

#include "MachineDefs.h"
#include "Debugger.h"
#include <cstdint>
#include <atomic>
#include <windows.h>
#include <assert.h>

//Speed throttling
#define FRAMEINTERVAL 120	//Number of frames to sum the framecounter over
#define TARGETFRAMERATE 60	//Number of throttled Frames per second to render
#define SAMPLESPERFRAME 262



//CPU 
#define FRAMESPERSECORD (double)59.923	//The coco really runs at about 59.923 Frames per second
#define LINESPERSCREEN (double)262
#define NANOSECOND (double)1000000000
#define COLORBURST (double)3579545 
#define AUDIOBUFFERS 12
//Misc
#define MAX_LOADSTRING 100
#define QUERY 255
#define INDEXTIME ((LINESPERSCREEN * TARGETFRAMERATE)/5)

namespace VCC
{
    const int DefaultWidth = 640;
    const int DefaultHeight = 480;

    struct Point
    {
        int x{}, y{};

        Point() {}
        Point(int x, int y) :x(x), y(y) {}
    };

    struct Size
    {
        int w{}, h{};

        Size() {}
        Size(int w, int h) :w(w), h(h) {}
    };

    struct Rect : Point, Size
    {
        Rect() {}
        Rect(int x, int y, int w, int h) : Point(x, y), Size(w, h) {}
        bool IsDefaultX() const { return x == CW_USEDEFAULT; }
        bool IsDefaultY() const { return y == CW_USEDEFAULT; }
        int Top() const { return IsDefaultY() ? 0 : y; }
        int Left() const { return IsDefaultX() ? 0 : x; }
        int Right() const { return Left() + w; }
        int Bottom() const { return Top() + h; }
        RECT GetRect() const
        {
            return { Left(), Top(), Right(), Bottom() };
        }
    };

    //
    // bounds checking array type
    //
    template <class T, size_t Size>
    struct Array
    {
        void MemCpyFrom(const void* buffer, size_t bytes)
        {
            assert(bytes <= sizeof(data));
            memcpy(&data[0], buffer, bytes);
        }

        void MemCpyTo(void* outBuffer, size_t bytes) 
        {
            MemCpyTo(0, outBuffer, bytes); 
        }

        void MemCpyTo(size_t startElement, void* outBuffer, size_t bytes)
        {
            assert(startElement*sizeof(T) + bytes <= sizeof(data));
            memcpy(outBuffer, &data[startElement], bytes);
        }

        void MemSet(uint8_t fill, size_t bytes)
        {
            assert(bytes <= sizeof(data));
            memset(data, fill, bytes);
        }

        T& operator[](size_t index) { return data[index]; }
        const T& operator[](size_t index) const { return data[index]; }

    private:
        std::array<T, Size> data;
    };


    //
    // protect T by verifying surrounding bytes
    //
#ifdef _DEBUG
    template <class T, size_t Size = 256>
    struct Protect
    {
        Protect()
        {
            for (int i = 0; i < Size; ++i)
                before[i] = after[i] = 0xDEADBEEF;
        }

        void Check()
        {
            for (int i = 0; i < Size; ++i)
            {
                assert(before[Size-i-1] == 0xDEADBEEF);
                assert(after[i] == 0xDEADBEEF);
            }
        }

    private:
        uint32_t before[Size];
    public:
        T Data;
    private:
        uint32_t after[Size];
    };
#else
    template <class T>
    struct Protect 
    {
        T Data; 
        void Check() {}
    };
#endif
}

struct SystemState
{
    HWND			WindowHandle;
    HWND			ConfigDialog;

    HINSTANCE		WindowInstance;
    unsigned char	*RamBuffer;
    unsigned short	*WRamBuffer;
    std::atomic<unsigned char>	RamSize;
    double			CPUCurrentSpeed;
    unsigned char	DoubleSpeedMultiplyer;
    unsigned char	DoubleSpeedFlag;
    unsigned char	TurboSpeedFlag;
    unsigned char	CpuType;
    unsigned char	FrameSkip;
    unsigned char	BitDepth;
    unsigned char	Throttle;
    unsigned char	*PTRsurface8;
    unsigned short	*PTRsurface16;
    unsigned int	*PTRsurface32;
    long			SurfacePitch;
    unsigned short	LineCounter;
    unsigned char	ScanLines;
    unsigned char	EmulationRunning;
    unsigned char	ResetPending;
    VCC::Rect		WindowSize;
    unsigned char	FullScreen;
    unsigned char	MousePointer;
    unsigned char	OverclockFlag;
    char			StatusLine[256];

	// Debugger Package	
	VCC::Debugger::Debugger Debugger;
};

extern SystemState EmuState;

static char RateList[4][7]={"Mute","11025","22050","44100"};
static unsigned int iRateList[4]={0,11025,22050,44100};

#endif
