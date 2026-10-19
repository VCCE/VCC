#ifndef __TCC1014GRAPHICS_H__
#define __TCC1014GRAPHICS_H__
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

#include "defines.h"
#include "Event.h"
#include <vector>

static unsigned char Lpf[4]={192,199,225,225}; // 2 is really undefined but I gotta put something here.
static unsigned char VcenterTable[4] = { 25,19,8,8 };
#if USE_NEWTIMING
static unsigned char TopOffScreenTable[4] = { 11,15,11,11 };
#else
static unsigned char TopOffScreenTable[4] = { 11,14,11,11 };
#endif
static unsigned char BottomOffScreenTable[4] = { 5,1,5,5 };

struct GimeGpu
{
	using Surface32 = VCC::VideoArray<unsigned int, 640 * 480>;

#if USE_NEWTIMING
	template <typename A, typename B> using Pair = std::pair<A, B>;
	template <typename T> using Vector = std::vector<T>;
	using UpdateFunction = std::function<void()>;
	using SyncFunction = std::function<void(int)>;

	GimeGpu(SyncFunction hSync, SyncFunction vSync);
#endif
	GimeGpu();

	unsigned int VidMask = 0x1FFFF;
	unsigned char VresIndex = 0;
	unsigned char CC2Offset = 0, CC2VDGMode = 0, CC2VDGPiaMode = 0;
	unsigned short VerticalOffsetRegister = 0;
	unsigned char CompatMode = 0;
	unsigned char MonType = 1;
	unsigned char CC3Vmode = 0, CC3Vres = 0, CC3BoarderColor = 0;
	unsigned int StartofVidram = 0, NewStartofVidram = 0;
	unsigned char LinesperScreen = 0;
	unsigned char Bpp = 0;
	unsigned char LinesperRow = 1, BytesperRow = 32;
	unsigned char GraphicsMode = 0;
	unsigned char TextFGColor = 0, TextBGColor = 0;
	unsigned char TextFGPalette = 0, TextBGPalette = 0;
	unsigned char PaletteIndex = 0;
	unsigned short PixelsperLine = 0, VPitch = 32;
	unsigned char Stretch = 0, PixelsperByte = 0;
	unsigned char HorzCenter = 0, VertCenter = 0;
	unsigned char LowerCase = 0, InvertAll = 0, ExtendedText = 1;
	unsigned char HorzOffsetReg = 0;
	unsigned char Hoffset = 0;
	unsigned char NewHoffset = 0;
	unsigned short TagY = 0;
	unsigned char VertScroll = 0;
	unsigned char UnderlineRow = 0;
	unsigned int BoarderColor32 = 0;
	unsigned short BoarderColor16 = 0;
	unsigned char BoarderColor8 = 0;
	unsigned int DistoOffset = 0;
#if !USE_NEWTIMING
	unsigned char BoarderChange = 3;
#endif
	unsigned char MasterMode = 0;
	unsigned char ColorInvert = 1;
	unsigned char BlinkState = 1;
	unsigned char TopBoarder = 0, BottomBoarder = 0, TopOffScreen = 0, BottomOffScreen = 0;
	bool UserFlipped = false;
	unsigned int last_mmode = 0;
	const char* curr_gmode = "";
	const char* last_gmode = "";
	unsigned char  Palette[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };		//Coco 3 6 bit colors
	unsigned char  Palette8Bit[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	unsigned short Palette16Bit[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };	//Color values translated to 16bit 32BIT
	unsigned int   Palette32Bit[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };	//Color values translated to 24/32 bits

	//This routine gets called every time a software video register get updated.
	void SetupDisplay();
	int Pmode4MonType() const;
	void TogBlinkState();

	unsigned int GetRowLine(int y)
	{
		auto vScroll = CompatMode == 0 ? VertScroll : 0;
		auto rowLine = (y + vScroll) % LinesperRow;
		return rowLine;
	}

	unsigned int GetAddressStart(int y)
	{
		auto vScroll = CompatMode == 0 ? VertScroll : 0;
		auto row = (y + vScroll) / LinesperRow;
		if ((CC3Vmode & 7) == 7) row = 0;
		return StartofVidram + (row * VPitch * ExtendedText);
	}

	unsigned int GetYStride(int y, unsigned int pitch)
	{
		return (((y + VertCenter) * 2) * pitch) + HorzCenter - 1;
	}

	unsigned char GetFontRowCC3(unsigned char ch, unsigned int line)
	{
		extern unsigned char cc3Fontdata8x12[];
		if (line > 11) return 0;
		return cc3Fontdata8x12[ch * 12 + line];
	}


	int GetBytesPerRow() const;
	int GetGraphicsMode() const;
	unsigned char GetHorizontalBorderSize() const;
	unsigned char SetMonitorType(unsigned char Type);
	unsigned char SetScanLines(unsigned char Lines);
	unsigned int GetStartOfVidram() const;
	unsigned short GetDisplayedPixelsPerLine() const;
	void SetLinesperScreen(unsigned char Lines);
	void FlipArtifacts();
	void GimeInit();
	void GimeReset();
#if !USE_NEWTIMING
	void InvalidateBoarder();
	void SetBoarderChange();
#endif
	void SetCompatMode(unsigned char Register);
	void SetGimeBoarderColor(unsigned char data);
	void SetGimeHorzOffset(unsigned char data);
	void SetGimePalette(unsigned char palette, unsigned char color);
	void SetGimeVdgMode(unsigned char VdgMode);
	void SetGimeVdgMode2(unsigned char Vdgmode2);
	void SetGimeVdgOffset(unsigned char Offset);
	void SetVerticalScroll(unsigned char scroll);
	void SetGimeVmode(unsigned char vmode);
	void SetGimeVres(unsigned char vres);
	void SetPaletteType();
	void SetVerticalOffsetRegister(unsigned short Register);
	void SetVideoBank(unsigned char data);
	void SetVidMask(unsigned int data);
	bool CopyPalette(const GimeGpu& other);
	void SetDefaultPalette();

	void DrawTopBoarder8(SystemState* DTState);
	void DrawTopBoarder16(SystemState* DTState);
	void DrawTopBoarder24(SystemState* /*DTState*/);
	void DrawTopBoarder32(SystemState* DTState);

	void DrawBottomBoarder8(SystemState* DTState);
	void DrawBottomBoarder16(SystemState* DTState);
	void DrawBottomBoarder24(SystemState* DTState);
	void DrawBottomBoarder32(SystemState* DTState);

	void UpdateScreen8(SystemState* US8State);
	void UpdateScreen16(SystemState* USState16);
	void UpdateScreen24(SystemState* /*USState24*/);
	void UpdateScreen32(SystemState* USState32);
	void UpdateScreen32To(unsigned char* buffer, unsigned int* surface, int lineCounter, int surfacePitch, bool scanLines);

#if USE_NEWTIMING
	void SetupColorSet(unsigned char set);

	// mark the beginning of a new line ready to capture events
	void BeginCpuLine();

	// finalize the current line
	void EndCpuLine();

	// push a change into the line history
	void PushLineUpdate(UpdateFunction func);

	// updates the timer value (on write to $ff94/$ff95)
	void SetTimer(unsigned int time);

	// restart the timer (on write to init1)
	void ResetTimer();

	// true when current frame has completed (for one call, auto resets)
	bool FrameFinished();

	// returns the time from start of the line
	VCC::Event::Clocks GetLineStartClock() { return mCpuLine.LineStartClock; }
#endif

private:
	void RenderNTSCPixel2x2(Surface32 surface32, size_t surfaceDest, int XpitchDest, char colorIndex, char scanLines);
	void RenderPMODE4NTSC(Surface32 surface32, size_t surfaceDest, int XpitchDest, const unsigned char* cocoSrc, char scanLines);

	static void MakeRGBPalette();
	static void MakeCMPpalette();

#if USE_NEWTIMING
	// retrigger timer interrupt after reaching zero
	void TimerRetrigger();

	// issue the timer interrupt, retriggers
	void TimerInterrupt(int delay);

	// decrement the timer counter and set interrupt at zero
	void DecrementTimerAndSet();

	// process changes up to pixel x coordinate
	void ProcessCpuLine(unsigned int pixelx);

	// draw regions (top, active, bottom)
	void DrawTopBorder();
	void DrawActiveArea();
	void DrawBottomBorder();

	// draw single raster line
	void DrawRaster();

	// when hsync/vsync falls
	void VSyncFall();
	void HSyncFall();

	// issue hsync/vsync interrupts
	void VSYNC(int level);
	void HSYNC(int level);

	// queue next hsync fall/rise event & gime hsync interrupt
	void HSyncQueueEvents(int offset = 0);

	static auto constexpr VIDEO_LINES = 263;
	static auto constexpr VSYNC_RISE = 4;

	static auto constexpr CLK_HSYNC = 80;
	static auto constexpr CLK_LINE = 912;
	static auto constexpr CLK_TIMER_IRQ_DELAY = 32;
	static auto constexpr CLK_TIMER_DECR_DELAY = 912 - 16;
	static auto constexpr CLK_VSYNC_RISE = CLK_LINE * VSYNC_RISE;

	struct CpuLineUpdates
	{
		size_t Index;
		Vector<Pair<VCC::Event::Clocks, UpdateFunction>> Data;
		VCC::Event::Clocks LineStartClock;
	};

	CpuLineUpdates mCpuLine;	// the line history
	int mClockVSyncFall;		// vsync fall time from last hsync fall
	int mClockHSync;			// hsync (gime)
	int mLeftEdge;				// pixels to left screen edge
	int mRightEdge;				// pixels to right screen edge
	int mLeftDraw;				// offset from real pixel to left screen edge
	size_t mLineCount;			// current line globally
	size_t mTimerCount;			// current timer countdown
	size_t mTimer;				// current timer value in lines
	bool mTimerSet;				// flag if timer interrupt is ready to fire
	bool mFrameFinished;		// flag when frame completed
	bool mImmediate;			// flag when instance is immediate mode
	SyncFunction mHSync;		// called on hsync
	SyncFunction mVSync;		// called on vsync
#endif
};

extern GimeGpu gGimeGpu;

#endif
