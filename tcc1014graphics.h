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

static unsigned char Lpf[4]={192,199,225,225}; // 2 is really undefined but I gotta put something here.
static unsigned char VcenterTable[4] = { 25,19,8,8 };
static unsigned char TopOffScreenTable[4] = { 11,14,11,11 };
static unsigned char BottomOffScreenTable[4] = { 5,1,5,5 };

struct GimeGpu
{
	using Surface32 = VCC::VideoArray<unsigned int, 640 * 480>;

	unsigned int VidMask = 0x1FFFF;
	unsigned char VresIndex = 0;
	unsigned char CC2Offset = 0, CC2VDGMode = 0, CC2VDGPiaMode = 0;
	unsigned short VerticalOffsetRegister = 0;
	unsigned char CompatMode = 0;
	unsigned char MonType = 1;
	unsigned char CC3Vmode = 0, CC3Vres = 0, CC3BoarderColor = 0;
	unsigned int StartofVidram = 0, Start = 0, NewStartofVidram = 0;
	unsigned char LinesperScreen = 0;
	unsigned char Bpp = 0;
	unsigned char LinesperRow = 1, BytesperRow = 32;
	unsigned char GraphicsMode = 0;
	unsigned char TextFGColor = 0, TextBGColor = 0;
	unsigned char TextFGPallete = 0, TextBGPallete = 0;
	unsigned char PalleteIndex = 0;
	unsigned short PixelsperLine = 0, VPitch = 32;
	unsigned char Stretch = 0, PixelsperByte = 0;
	unsigned char HorzCenter = 0, VertCenter = 0;
	unsigned char LowerCase = 0, InvertAll = 0, ExtendedText = 1;
	unsigned char HorzOffsetReg = 0;
	unsigned char Hoffset = 0;
	unsigned short TagY = 0;
	unsigned int BoarderColor32 = 0;
	unsigned short BoarderColor16 = 0;
	unsigned char BoarderColor8 = 0;
	unsigned int DistoOffset = 0;
	unsigned char BoarderChange = 3;
	unsigned char MasterMode = 0;
	unsigned char ColorInvert = 1;
	unsigned char BlinkState = 1;
	bool UserFlipped = false;
	unsigned int last_mmode = 0;
	const char* curr_gmode = "";
	const char* last_gmode = "";


	//This routine gets called every time a software video register get updated.
	void SetupDisplay();
	int Pmode4MonType() const;
	void TogBlinkState();

	int GetBytesPerRow() const;
	int GetGraphicsMode() const;
	unsigned char GetHorizontalBorderSize() const;
	unsigned char SetMonitorType(unsigned char Type);
	unsigned char SetScanLines(unsigned char Lines);
	unsigned int GetStartOfVidram() const;
	unsigned short GetDisplayedPixelsPerLine() const;
	void FlipArtifacts();
	void GimeInit();
	void GimeReset();
	void InvalidateBoarder();
	void SetBoarderChange();
	void SetCompatMode(unsigned char Register);
	void SetGimeBoarderColor(unsigned char data);
	void SetGimeHorzOffset(unsigned char data);
	void SetGimePalette(unsigned char pallete, unsigned char color) const;
	void SetGimeVdgMode(unsigned char VdgMode);
	void SetGimeVdgMode2(unsigned char Vdgmode2);
	void SetGimeVdgOffset(unsigned char Offset);
	void SetGimeVmode(unsigned char vmode);
	void SetGimeVres(unsigned char vres);
	void SetPaletteType();
	void SetVerticalOffsetRegister(unsigned short Register);
	void SetVideoBank(unsigned char data);
	void SetVidMask(unsigned int data);

	void DrawTopBoarder8(SystemState* DTState) const;
	void DrawTopBoarder16(SystemState* DTState) const;
	void DrawTopBoarder24(SystemState* /*DTState*/) const;
	void DrawTopBoarder32(SystemState* DTState) const;

	void DrawBottomBoarder8(SystemState* DTState) const;
	void DrawBottomBoarder16(SystemState* DTState) const;
	void DrawBottomBoarder24(SystemState* DTState) const;
	void DrawBottomBoarder32(SystemState* DTState) const;

	void UpdateScreen8(SystemState* US8State);
	void UpdateScreen16(SystemState* USState16);
	void UpdateScreen24(SystemState* /*USState24*/);
	void UpdateScreen32(SystemState* USState32);
	void UpdateScreen32To(unsigned char* buffer, unsigned int* surface, int lineCounter, int surfacePitch, bool scanLines);

private:
	void RenderNTSCPixel2x2(Surface32 surface32, size_t surfaceDest, int XpitchDest, char colorIndex, char scanLines) const;
	void RenderPMODE4NTSC(Surface32 surface32, size_t surfaceDest, int XpitchDest, const unsigned char* cocoSrc, char scanLines) const;

	static void MakeRGBPalette();
	static void MakeCMPpalette();
};

extern GimeGpu gGimeGpu;

#endif
