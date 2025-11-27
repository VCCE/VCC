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
#include <Windows.h>
#include <assert.h>

constexpr auto MAX_LOADSTRING = 400u;

//Speed throttling
constexpr auto FRAMEINTERVAL = 120u;	//Number of frames to sum the framecounter over
constexpr auto TARGETFRAMERATE = 60u;	//Number of throttled Frames per second to render
constexpr auto SAMPLESPERFRAME = 262u;

#pragma warning(disable: 4100)


//CPU 
constexpr auto FRAMESPERSECORD = 59.923;	//The coco really runs at about 59.923 Frames per second
constexpr auto LINESPERSCREEN = 262.0;
constexpr auto NANOSECOND = 1000000000.0;
constexpr auto COLORBURST = 3579545.0;
constexpr auto AUDIOBUFFERS = 12u;
//Misc
constexpr auto QUERY = 255u;

struct SystemState;

namespace VCC
{
    const int DefaultWidth = 640;
    const int DefaultHeight = 480;

    union Pixel;

    struct Point
    {
        int x{}, y{};

        Point() = default;
        Point(int x, int y) :x(x), y(y) {}
    };

    struct Size
    {
        int w{}, h{};

        Size() = default;
        Size(int w, int h) :w(w), h(h) {}
    };

    struct Rect : Point, Size
    {
        Rect() = default;
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

	// FIXME: Remove this and use std::array
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
    // bounds checking on existing array 
    // NOTE: only for video surface, out-of-bounds wraps
    //
    template <class T, size_t Size>
    struct VideoArray
    {    
		explicit VideoArray(T* p) : data(p) {}

        T& operator[](size_t index) 
        {
            assert(index < Size);
            return data[index % Size];
        }
        
        const T& operator[](size_t index) const 
        { 
            assert(index < Size);
            return data[index % Size]; 
        }

    private:
        T* data;
    };


    // delayed flip flop
    struct DFF 
    {
        uint8_t D{}; // d input
        uint8_t Q{}; // q output

        void Reset()
        {
            D = Q = 0;
        }

        void Clock(uint8_t d)
        {
            Q = D;
            D = d;
        }
    };

    //
    // abstract interface for system state
    //
    struct ISystemState
    {
        enum { OK };
		virtual ~ISystemState() = default;

        virtual int GetWindowHandle(void** handle) = 0;
        virtual int GetRect(int rectOption, Rect* rect) = 0;
        virtual void SetSurface(void* ptr, uint8_t bitDepth, long stride) = 0;
    };
}

struct SystemState
{
    HWND			WindowHandle = nullptr;
    HWND			ConfigDialog = nullptr;

    HINSTANCE		WindowInstance = nullptr;	//	This is named wrong. no fucking surprise. ModuleHandle or ModuleInstance or ModuleInstanceHandle!
    unsigned char	*RamBuffer = nullptr;
    unsigned short	*WRamBuffer = nullptr;
    std::atomic<unsigned char>	RamSize = 0;
    double			CPUCurrentSpeed = 0.0;
    unsigned char	DoubleSpeedMultiplyer = 0;
    unsigned char	DoubleSpeedFlag = 0;
    unsigned char	TurboSpeedFlag = 0;
    unsigned char	CpuType = 0;
    unsigned char	FrameSkip = 0;
    unsigned char	BitDepth = 0;
    unsigned char	Throttle = 0;
    unsigned char	*PTRsurface8 = nullptr;
    unsigned short	*PTRsurface16 = nullptr;
    unsigned int	*PTRsurface32 = nullptr;
    long			SurfacePitch = 0;
    unsigned short	LineCounter = 0;
    unsigned char	ScanLines = 0;
    unsigned char	EmulationRunning = 0;
    unsigned char	ResetPending = 0;
    VCC::Size		WindowSize;
    unsigned char	FullScreen = 0;
    bool        	Exiting = false;
    unsigned char	MousePointer = 0;
    unsigned char	OverclockFlag = 0;
	char			StatusLine[256] = { 0 };

	// Debugger Package	
	VCC::Debugger::Debugger Debugger;
};


//
// Abstract interface over SystemState that the display classes use
//
struct SystemStatePtr : VCC::ISystemState
{
	explicit SystemStatePtr(SystemState* state) : state(state) {}

    int GetWindowHandle(void** handle) override 
    { 
        *handle = state->WindowHandle; 
        return OK; 
    }
    int GetRect(int rectOption, VCC::Rect* rect) override 
    {
        rect->x = 0; rect->y = 0; rect->w = state->WindowSize.w; rect->h = state->WindowSize.h; 
        return OK; 
    }
    void SetSurface(void* ptr, uint8_t bitDepth, long stride) override
    {
        state->PTRsurface8 = (unsigned char*)ptr;
        state->PTRsurface16 = (unsigned short*)ptr;
        state->PTRsurface32 = (unsigned int*)ptr;
        state->BitDepth = bitDepth;
        state->SurfacePitch = stride;
    }

private:
    SystemState* state;
};

extern SystemState EmuState;

static char RateList[4][7]={"Mute","11025","22050","44100"};
static unsigned int iRateList[4]={0,11025,22050,44100};

#endif
