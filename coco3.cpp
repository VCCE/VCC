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

#include "Windows.h"
#include "stdio.h"
#include "ddraw.h"
#include <math.h>
#include "defines.h"
#include "BuildConfig.h"
#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "mc6821.h"
#include "hd6309.h"
#include "mc6809.h"
#include "pakinterface.h"
#include "audio.h"
#include "coco3.h"
#include "throttle.h"
#include "Vcc.h"
#include "Cassette.h"
#include "DirectDrawInterface.h"
#include <vcc/core/logger.h>
#include "keyboard.h"
#include <string>
#include <iostream>
#include "config.h"
#include "tcc1014mmu.h"


#if USE_DEBUG_AUDIOTAPE
#include "IDisplayDebug.h"
const int AudioHistorySize = 900;
struct AudioHistory
{
	char motorState;
	char audioState;
	int inputMin;
	int inputMax;
};
AudioHistory gAudioHistory[AudioHistorySize];
int gAudioHistoryCount = 0;
#endif

constexpr auto RENDERS_PER_BLINK_TOGGLE = 16u;

//****************************************
	static double SoundInterupt=0;
	static double NanosToSoundSample=SoundInterupt;
	static double NanosToAudioSample = SoundInterupt;
	static double CyclesPerSecord=(COLORBURST/4)*(TARGETFRAMERATE/FRAMESPERSECORD);
	static double LinesPerSecond= TARGETFRAMERATE * LINESPERSCREEN;
	static double NanosPerLine = NANOSECOND / LinesPerSecond;
	static double HSYNCWidthInNanos = 5000;
	static double CyclesPerLine = CyclesPerSecord / LinesPerSecond;
	static double CycleDrift=0;
	static double CyclesThisLine=0;
	static unsigned int StateSwitch=0;
	unsigned int SoundRate=0;
//*****************************************************

static unsigned char HorzInteruptEnabled=0,VertInteruptEnabled=0;
static unsigned char TopBoarder=0,BottomBoarder=0,TopOffScreen=0,BottomOffScreen=0;
static unsigned char LinesperScreen;
static unsigned char TimerInteruptEnabled=0;
static int MasterTimer=0; 
static unsigned int TimerClockRate=0;
static int TimerCycleCount=0;
static double MasterTickCounter=0,UnxlatedTickCounter=0,OldMaster=0;
static double NanosThisLine=0;
static unsigned char BlinkPhase=1;
static unsigned int AudioBuffer[16384];
static unsigned char CassBuffer[8192];
static unsigned int AudioIndex = 0;
static unsigned int CassIndex = 0;
static unsigned int CassBufferSize = 0;
double NanosToInterrupt=0;
static int IntEnable=0;
static int SndEnable=1;
static int OverClock=1;
static unsigned char SoundOutputMode=0;	//Default to Speaker 1= Cassette
static double emulatedCycles;
double TimeToHSYNCLow = 0;
double TimeToHSYNCHigh = 0;
static unsigned char LastMotorState;
static int AudioFreeBlockCount;

static int clipcycle = 1, cyclewait=2000;
bool codepaste, PasteWithNew = false; 
void AudioOut();
void CassOut();
void CassIn();
void (*AudioEvent)()=AudioOut;
void SetMasterTickCounter();
void (*DrawTopBoarder[4]) (SystemState *)={DrawTopBoarder8,DrawTopBoarder16,DrawTopBoarder24,DrawTopBoarder32};
void (*DrawBottomBoarder[4]) (SystemState *)={DrawBottomBoarder8,DrawBottomBoarder16,DrawBottomBoarder24,DrawBottomBoarder32};
void (*UpdateScreen[4]) (SystemState *)={UpdateScreen8,UpdateScreen16,UpdateScreen24,UpdateScreen32};
std::string GetClipboardText();
void HLINE();
void VSYNC(unsigned char level);
void HSYNC(unsigned char level);
std::string CvtStrToSC(std::string);

using namespace std;

_inline void CPUCycle(double nanoseconds);

void UpdateAudio()
{
#if USE_DEBUG_AUDIOTAPE
	if (CassIndex < CassBufferSize)
	{
		uint8_t sample = LastMotorState ? CassBuffer[CassIndex] : CAS_SILENCE;
		auto& data = gAudioHistory[AudioHistorySize - 1];
		if (gAudioHistoryCount == 0)
		{
			data.inputMin = 0;
			data.inputMax = 0;
		}
		if (gAudioHistoryCount < 5)
		{
			data.inputMin += sample;
			data.inputMax += sample;
		}
		else if (gAudioHistoryCount == 5)
		{
			// begin with average sample (last 6)
			data.inputMin += sample;
			data.inputMax += sample;
			data.inputMin /= 6;
			data.inputMax /= 6;
		}
		else
		{
			// now, keep track of peak to peak volume
			if (sample > data.inputMax) data.inputMax = sample;
			if (sample < data.inputMin) data.inputMin = sample;
		}
		++gAudioHistoryCount;
	}
#endif // USE_DEBUG_AUDIOTAPE

	// keep audio system full by tiny expansion of sound
	if (AudioFreeBlockCount > 1 && (AudioIndex & 63) == 1)
	{
		unsigned int last = AudioBuffer[AudioIndex - 1];
		AudioBuffer[AudioIndex++] = last;
	}
}

void DebugDrawAudio()
{
#if USE_DEBUG_AUDIOTAPE
	static VCC::Pixel col(255, 255, 255);
	col.a = 240;
	for (int i = 0; i < 734; ++i)
	{
		DebugDrawLine(i, 256 - ((AudioBuffer[i] & 0xFFFF) >> 7), i + 1, 256 - ((AudioBuffer[i + 1] & 0xFFFF) >> 7), col);
		DebugDrawLine(750+i, 256 - ((AudioBuffer[i] & 0xFFFF0000) >> 23), 750+i + 1, 256 - ((AudioBuffer[i + 1] & 0xFFFF00000) >> 23), col);
	}

	auto& history = gAudioHistory[AudioHistorySize - 1];
	history.motorState = LastMotorState;
	history.audioState = GetMuxState() == PIA_MUX_CASSETTE;
	gAudioHistoryCount = 0;
	for (int i = 0; i < AudioHistorySize - 1; ++i)
	{
		auto& a = gAudioHistory[i];
		auto& b = gAudioHistory[i + 1];
		DebugDrawLine(i, 500 - a.inputMin, i + 1, 500 - a.inputMax, col);
		DebugDrawLine(i, 520 - 20 * a.motorState, i + 1, 520 - 20 * b.motorState, col);
		DebugDrawLine(i, 550 - 20 * a.audioState, i + 1, 550 - 20 * b.audioState, col);
	}
	memcpy(&gAudioHistory[0], &gAudioHistory[1], sizeof(gAudioHistory) - sizeof(*gAudioHistory));
#endif // USE_DEBUG_AUDIOTAPE
}


float RenderFrame (SystemState *RFState)
{
	static unsigned int FrameCounter=0;

	// once per frame
	LastMotorState = GetMotorState();
	AudioFreeBlockCount = GetFreeBlockCount();

//********************************Start of frame Render*****************************************************

	// Blink state toggle
	if (BlinkPhase++ > RENDERS_PER_BLINK_TOGGLE) {
		TogBlinkState();
		BlinkPhase = 0;
	}

	// VSYNC goes Low
	VSYNC(0);

	// Four lines of blank during VSYNC low
	for (RFState->LineCounter = 0; RFState->LineCounter < 4; RFState->LineCounter++)
	{
		HLINE();
	}

	// VSYNC goes High
	VSYNC(1);

	// Three lines of blank after VSYNC goes high
	for (RFState->LineCounter = 0; RFState->LineCounter < 3; RFState->LineCounter++)
	{
		HLINE();
	}

	// Top Border actually begins here, but is offscreen
	for (RFState->LineCounter = 0; RFState->LineCounter < TopOffScreen; RFState->LineCounter++)
	{
		HLINE();
	}

	if (!(FrameCounter % RFState->FrameSkip))
	{
		if (LockScreen())
			return 0;
	}

	// Visible Top Border begins here. (Remove 4 lines for centering)
	RFState->Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenTopBorder, 0);
	for (RFState->LineCounter = 0; RFState->LineCounter < TopBoarder; RFState->LineCounter++)
	{
		HLINE();
		if (!(FrameCounter % RFState->FrameSkip))
			DrawTopBoarder[RFState->BitDepth](RFState);
	}

	// Main Screen begins here: LPF = 192, 200 (actually 199), 225
	RFState->Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenRender, 0);
	for (RFState->LineCounter = 0; RFState->LineCounter < LinesperScreen; RFState->LineCounter++)		
	{
		HLINE();
		if (!(FrameCounter % RFState->FrameSkip))
			UpdateScreen[RFState->BitDepth](RFState);
	}

	// Bottom Border begins here.
	RFState->Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenBottomBorder, 0);
	for (RFState->LineCounter=0;RFState->LineCounter < BottomBoarder;RFState->LineCounter++)
	{
		HLINE();
		if (!(FrameCounter % RFState->FrameSkip))
			DrawBottomBoarder[RFState->BitDepth](RFState);
	}

	if (!(FrameCounter % RFState->FrameSkip))
	{
		DrawBottomBoarder[RFState->BitDepth](RFState);
		UnlockScreen(RFState);
		SetBoarderChange();
	}

	// Bottom Border continues but is offscreen
	for (RFState->LineCounter = 0; RFState->LineCounter < BottomOffScreen; RFState->LineCounter++)
	{
		HLINE();
	}

	switch (SoundOutputMode)
	{
	case 1:
		FlushCassetteBuffer(CassBuffer,&CassIndex);
		break;
	}
	FlushAudioBuffer(AudioBuffer, AudioIndex << 2);
	AudioIndex=0;

	DebugDrawAudio();

	// Only affect frame rate if a debug window is open.
	RFState->Debugger.Update();

	return(CalculateFPS());
}

void VSYNC(unsigned char level)
{
	if (level == 0)
	{
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenVSYNCLow, 0);
		irq_fs(0);
		if (VertInteruptEnabled)
			GimeAssertVertInterupt();
	}
	else
	{
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenVSYNCHigh, 0);
		irq_fs(1);
	}
}

void HSYNC(unsigned char level)
{
	if (level == 0)
	{
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenHSYNCLow, 0);
		if (HorzInteruptEnabled)
			GimeAssertHorzInterupt();
		irq_hs(0);
	}
	else
	{
		EmuState.Debugger.TraceCaptureScreenEvent(VCC::TraceEvent::ScreenHSYNCHigh, 0);
		irq_hs(1);
	}
}

void SetClockSpeed(unsigned int Cycles)
{
	OverClock=Cycles;
	return;
}

void SetHorzInteruptState(unsigned char State)
{
	HorzInteruptEnabled= !!State;
	return;
}

void SetVertInteruptState(unsigned char State)
{
	VertInteruptEnabled= !!State;
	return;
}

void SetLinesperScreen (unsigned char Lines)
{
	Lines = (Lines & 3);
	LinesperScreen=Lpf[Lines];
	TopBoarder=VcenterTable[Lines];
	BottomBoarder = 239 - (TopBoarder + LinesperScreen);
	TopOffScreen = TopOffScreenTable[Lines];
	BottomOffScreen = BottomOffScreenTable[Lines];
	return;
}


DisplayDetails GetDisplayDetails(const int clientWidth, const int clientHeight)
{
	const float pixelsPerLine = GetDisplayedPixelsPerLine();
	const float horizontalBorderSize = GetHorizontalBorderSize();
	const float activeLines = 192.0f;	//	FIXME: Needs a symbolic

	DisplayDetails details;

	const auto extraBorderPadding = GetForcedAspectBorderPadding();

	// calcuate the complete screen size including its borders in device coords
	float deviceScreenWidth = (float)clientWidth - extraBorderPadding.x * 2;
	float deviceScreenHeight = (float)clientHeight - extraBorderPadding.y * 2;

	// calculate the content size including the borders in surface coords
	float contentWidth = pixelsPerLine + horizontalBorderSize * 2;
	float contentHeight = activeLines + TopBoarder + BottomBoarder;

	// now get scale difference between both previous equivalent boxes
	float horizontalScale = deviceScreenWidth / contentWidth;
	float verticalScale = deviceScreenHeight / contentHeight;

	// fill in details by scalling the coco screen into device coords
	details.contentRows = static_cast<int>(LinesperScreen * verticalScale);
	details.topBorderRows = static_cast<int>(TopBoarder * verticalScale) + extraBorderPadding.y;
	details.bottomBorderRows = static_cast<int>(BottomBoarder * verticalScale) + extraBorderPadding.y;

	details.contentColumns = static_cast<int>(pixelsPerLine * horizontalScale);
	details.leftBorderColumns = static_cast<int>(horizontalBorderSize * horizontalScale) + extraBorderPadding.x;
	details.rightBorderColumns = static_cast<int>(horizontalBorderSize * horizontalScale) + extraBorderPadding.x;
	
	return details;
}

_inline void HLINE()
{
	UpdateAudio();

	// First part of the line
	CPUCycle(NanosPerLine - HSYNCWidthInNanos);

	// HSYNC going low.
	HSYNC(0);
	PakTimer();

	// Run for a bit.
	CPUCycle(HSYNCWidthInNanos);

	// HSYNC goes high
	HSYNC(1);
}

_inline void CPUCycle(double NanosToRun)
{
	// CPU is in a halted state.
	if (EmuState.Debugger.IsHalted())
	{
		return;
	}

	EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 10, NanosToRun, 0, 0, 0, 0);
	NanosThisLine += NanosToRun;
	double emulationCycles = 0, emulationDrift = 0;
	while (NanosThisLine >= 1)
	{
		StateSwitch = 0;
		if ((NanosToInterrupt <= NanosThisLine) & IntEnable)	//Does this iteration need to Timer Interupt
			StateSwitch = 1;
		if ((NanosToSoundSample <= NanosThisLine) & SndEnable)//Does it need to collect an Audio sample
			StateSwitch += 2;
		switch (StateSwitch)
		{
		case 0:		//No interupts this line
			CyclesThisLine = CycleDrift + (NanosThisLine * CyclesPerLine * OverClock / NanosPerLine);
			if (CyclesThisLine >= 1)	//Avoid un-needed CPU engine calls
				CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
			else
				CycleDrift = CyclesThisLine;
			EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, StateSwitch, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
			emulationCycles += CyclesThisLine;
			emulationDrift += CycleDrift;
			NanosToInterrupt -= NanosThisLine;
			NanosToSoundSample -= NanosThisLine;
			NanosThisLine = 0;
			break;

		case 1:		//Only Interupting
			NanosThisLine -= NanosToInterrupt;
			CyclesThisLine = CycleDrift + (NanosToInterrupt * CyclesPerLine * OverClock / NanosPerLine);
			if (CyclesThisLine >= 1)
				CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
			else
				CycleDrift = CyclesThisLine;
			EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, StateSwitch, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
			emulationCycles += CyclesThisLine;
			emulationDrift += CycleDrift;
			GimeAssertTimerInterupt();
			NanosToSoundSample -= NanosToInterrupt;
			NanosToInterrupt = MasterTickCounter;
			break;

		case 2:		//Only Sampling
			NanosThisLine -= NanosToSoundSample;
			CyclesThisLine = CycleDrift + (NanosToSoundSample * CyclesPerLine * OverClock / NanosPerLine);
			if (CyclesThisLine >= 1)
				CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
			else
				CycleDrift = CyclesThisLine;
			EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, StateSwitch, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
			emulationCycles += CyclesThisLine;
			emulationDrift += CycleDrift;
			AudioEvent();
			NanosToInterrupt -= NanosToSoundSample;
			NanosToSoundSample = SoundInterupt;
			break;

		case 3:		//Interupting and Sampling
			if (NanosToSoundSample < NanosToInterrupt)
			{
				NanosThisLine -= NanosToSoundSample;
				CyclesThisLine = CycleDrift + (NanosToSoundSample * CyclesPerLine * OverClock / NanosPerLine);
				if (CyclesThisLine >= 1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				else
					CycleDrift = CyclesThisLine;
				EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 3, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
				emulationCycles += CyclesThisLine;
				emulationDrift += CycleDrift;
				AudioEvent();
				NanosToInterrupt -= NanosToSoundSample;
				NanosToSoundSample = SoundInterupt;
				NanosThisLine -= NanosToInterrupt;

				CyclesThisLine = CycleDrift + (NanosToInterrupt * CyclesPerLine * OverClock / NanosPerLine);
				if (CyclesThisLine >= 1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				else
					CycleDrift = CyclesThisLine;
				EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 4, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
				emulationCycles += CyclesThisLine;
				emulationDrift += CycleDrift;
				GimeAssertTimerInterupt();
				NanosToSoundSample -= NanosToInterrupt;
				NanosToInterrupt = MasterTickCounter;
				break;
			}

			if (NanosToSoundSample > NanosToInterrupt)
			{
				NanosThisLine -= NanosToInterrupt;
				CyclesThisLine = CycleDrift + (NanosToInterrupt * CyclesPerLine * OverClock / NanosPerLine);
				if (CyclesThisLine >= 1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				else
					CycleDrift = CyclesThisLine;
				EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 5, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
				emulationCycles += CyclesThisLine;
				emulationDrift += CycleDrift;
				GimeAssertTimerInterupt();
				NanosToSoundSample -= NanosToInterrupt;
				NanosToInterrupt = MasterTickCounter;
				NanosThisLine -= NanosToSoundSample;
				CyclesThisLine = CycleDrift + (NanosToSoundSample * CyclesPerLine * OverClock / NanosPerLine);
				if (CyclesThisLine >= 1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				else
					CycleDrift = CyclesThisLine;
				EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 6, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
				emulationCycles += CyclesThisLine;
				emulationDrift += CycleDrift;
				AudioEvent();
				NanosToInterrupt -= NanosToSoundSample;
				NanosToSoundSample = SoundInterupt;
				break;
			}
			//They are the same (rare)
			NanosThisLine -= NanosToInterrupt;
			CyclesThisLine = CycleDrift + (NanosToSoundSample * CyclesPerLine * OverClock / NanosPerLine);
			if (CyclesThisLine > 1)
				CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
			else
				CycleDrift = CyclesThisLine;
			EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 7, NanosThisLine, NanosToInterrupt, NanosToSoundSample, CyclesThisLine, CycleDrift);
			emulationCycles += CyclesThisLine;
			emulationDrift += CycleDrift;
			GimeAssertTimerInterupt();
			AudioEvent();
			NanosToInterrupt = MasterTickCounter;
			NanosToSoundSample = SoundInterupt;
		}
	}
	EmuState.Debugger.TraceEmulatorCycle(VCC::TraceEvent::EmulatorCycle, 20, 0, 0, 0, emulationCycles, emulationDrift);
}

void SetTimerInteruptState(unsigned char State)
{
	TimerInteruptEnabled=State;
	return;
}

void SetInteruptTimer(unsigned int Timer)
{
	UnxlatedTickCounter=(Timer & 0xFFF);
	SetMasterTickCounter();
	IntEnable = 1;  // Gime always sets timer flag when timer expires.  EJJ 25oct24
	return;
}

void SetTimerClockRate (unsigned char Tmp)	//1= 279.265nS (1/ColorBurst) 
{											//0= 63.695uS  (1/60*262)  1 scanline time
	TimerClockRate=!!Tmp;
	SetMasterTickCounter();
	return;
}

void SetMasterTickCounter()
{
	// Rate = { 63613.2315, 279.265 };
	double Rate[2]={NANOSECOND/(TARGETFRAMERATE*LINESPERSCREEN),NANOSECOND/COLORBURST};
	// Master count contains at least one tick. EJJ 10mar25
	MasterTickCounter = (UnxlatedTickCounter+1) * Rate[TimerClockRate];
	if (MasterTickCounter != OldMaster)  
	{
		OldMaster=MasterTickCounter;
		NanosToInterrupt=MasterTickCounter;
	}
	return;
}

void MiscReset()
{
	HorzInteruptEnabled=0;
	VertInteruptEnabled=0;
	TimerInteruptEnabled=0;
	MasterTimer=0; 
	TimerClockRate=0;
	MasterTickCounter=0;
	UnxlatedTickCounter=0;
	OldMaster=0;
//*************************
	SoundInterupt=0;//PICOSECOND/44100;
	NanosToSoundSample=SoundInterupt;
	NanosToAudioSample = SoundInterupt;
	CycleDrift=0;
	CyclesThisLine=0;
	NanosThisLine=0;
	IntEnable=0;
	AudioIndex=0;
	ResetAudio();
	return;
}

unsigned int SetAudioRate (unsigned int Rate)
{

	SndEnable=1;
	SoundInterupt=0;
	CycleDrift=0;

	if (Rate==0)
		SndEnable=0;
	else
	{
		SoundInterupt=NANOSECOND/Rate;
		NanosToSoundSample=SoundInterupt;
		NanosToAudioSample = NANOSECOND/AUDIO_RATE;
	}
	SoundRate=Rate;
	return 0;
}

void AudioOut()
{

	AudioBuffer[AudioIndex++]=GetDACSample();
	return;
}

void CassOut()
{
	if (LastMotorState && CassIndex < sizeof(CassBuffer)/sizeof(*CassBuffer))
		CassBuffer[CassIndex++]=GetCasSample();
	return;
}

//
// Reads the next byte from cassette data stream until end of the tape
// either fast mode, normal or wave data.
//
uint8_t CassInByteStream()
{
	if (CassIndex >= CassBufferSize)
	{
		LoadCassetteBuffer(CassBuffer, &CassBufferSize);
		CassIndex = 0;
	}
	return LastMotorState ? CassBuffer[CassIndex++] : CAS_SILENCE;
}

//
// In fast load mode, byte stream is two samples per bit high & low,
// the type of bit (0 or 1) depends on the period. based on this pass
// the period to basic at address $83.
//
uint8_t CassInBitStream()
{
	uint8_t nextHalfBit = CassInByteStream();
	// set counter time for one lo-hi cycle, basic checks for >18 (0bit) or <18 (1bit)
	MemWrite8(nextHalfBit & 1 ? 10 : 20, 0x83);
	return nextHalfBit >> 1;
}

void CassIn()
{
	// fade ramp state
	static unsigned int fadeTo = 0;
	static unsigned int fade = 0;

	// extract left channel
	auto getLeft = [](auto sample) { return (unsigned long)(sample & 0xFFFF); };
	// extract right channel
	auto getRight = [](auto sample) { return (unsigned long)((sample >> 16) & 0xFFFF); };
	// convert 8 bit to 16 bit stereo (like dac)
	auto monoToStereo = [](uint8_t sample) { return ((uint32_t)sample << 23) | ((uint32_t)sample << 7); };

	if (TapeFastLoad)
	{
		AudioBuffer[AudioIndex++] = GetMuxState() == PIA_MUX_CASSETTE ? monoToStereo(CAS_SILENCE) : GetDACSample();
	}
	else
	{
		// read next sample or same as last if motor is off
		auto casSample =  CassInByteStream();
		SetCassetteSample(casSample);

		// mix two channels dependant on mux (2 x 16bit)
		auto casChannel = monoToStereo(casSample);
		auto dacChannel = GetDACSample();

		// fade time of 125ms, note: this is slow enough to eliminate switching pop but
		// it must be quick too because some games have a nasty habit of toggling the 
		// mux on and off, such as Tuts Tomb.
		const int FADE_TIME = SoundRate / 8;

		// update ramp, always moving towards correct channel  
		fade = fade < fadeTo ? fade + 1 : fade > fadeTo ? fade - 1 : fade;

		// if mux changed, start transition
		fadeTo = FADE_TIME * (GetMuxState() == PIA_MUX_CASSETTE ? 1 : 0);

		// mix audio level between device channels
		auto left = (getLeft(casChannel) * fade + getLeft(dacChannel) * (FADE_TIME - fade)) / FADE_TIME;
		auto right = (getRight(casChannel) * fade + getRight(dacChannel) * (FADE_TIME - fade)) / FADE_TIME;
		auto sample = (unsigned int)(left + (right << 16));

		while (NanosToAudioSample > 0)
		{
			AudioBuffer[AudioIndex++] = sample;
			NanosToAudioSample -= NANOSECOND / AUDIO_RATE;
		}
		NanosToAudioSample += SoundInterupt;
	}
}



void SetSndOutMode(unsigned char Mode)  //0 = Speaker 1= Cassette Out 2=Cassette In
{
	static unsigned char LastMode=0;

	if (Mode == LastMode) return;

	switch (Mode)
	{
	case 0:
		if (LastMode==1)	//Send the last bits to be encoded
			FlushCassetteBuffer(CassBuffer,&CassIndex);

		AudioEvent=AudioOut;
		SetAudioRate(SoundRate);
		break;

	case 1:
		AudioEvent=CassOut;
		SetAudioRate(GetTapeRate());
		break;

	case 2:
		AudioEvent=CassIn;
		SetAudioRate(GetTapeRate());
		break;
	}

	LastMode=Mode;
	SoundOutputMode=Mode;
}

void PasteText() {
	using namespace std;
	std::string tmp;
	string cliptxt, clipparse, lines, debugout;
	int GraphicsMode = GetGraphicsMode();
	if (GraphicsMode != 0) {
		int tmp = MessageBox(nullptr, "Warning: You are not in text mode. Continue Pasting?", "Clipboard", MB_YESNO);
		if (tmp != 6) { return; }
	}

	cliptxt = GetClipboardText().c_str();
	if (PasteWithNew) { cliptxt = "NEW\n" + cliptxt; }
	char prvchr = '\0';
	for (size_t t = 0; t < cliptxt.length(); t++) {
		char tmp = cliptxt[t];
		bool eol;                   //EJJ CRLF,LFCR,CR,LF eol logic
		if (tmp == '\x0A') {        //LF
			if (prvchr == '\x0D') continue;
			eol = TRUE;
		} else if (tmp == '\x0D') { //CR
			if (prvchr == '\x0A') continue;
			eol = TRUE;
		} else {
			eol = FALSE;
		}
		prvchr = tmp;
		if (! eol) {
			lines += tmp;
		}
		else  { //...the character is a <CR>
			if (lines.length() > 249 && lines.length() < 257 && codepaste==true) {
				int b = lines.find(" ");
				string main = lines.substr(0, 249);
				string extra = lines.substr(249, lines.length() - 249);
				string spaces;
				for (int p = 1; p < 249; p++) {
					spaces.append(" ");
				}
				string linestr = lines.substr(0, b);
				lines = main+"\n\nEDIT "+linestr+"\n"+spaces+"I"+extra+"\n";
				clipparse += lines;
				lines.clear();
			}
			if(lines.length() >= 257 && codepaste==true) {
				// Line is too long to handle. Truncate.
				int b = lines.find(" ");
				string linestr = "Warning! Line "+lines.substr(0, b)+" is too long for BASIC and will be truncated.";
				
				MessageBox(nullptr, linestr.c_str(), "Clipboard", 0);
				lines = (lines.substr(0, 249));
			}
			if(lines.length() <= 249 || codepaste==false) { 
				// Just a regular line.
				clipparse += lines+"\n"; 
				lines.clear();
			}
		}
		if (t == cliptxt.length()-1) {
			clipparse += lines;
		}
	}
	PasteIntoQueue(CvtStrToSC(clipparse));
}

void QueueText(const char * text) {
	using namespace std;
	std::string str(text);
	PasteIntoQueue(CvtStrToSC(str));
}

std::string CvtStrToSC(string cliptxt)
{
	std::string out;
	char sc;
	char letter;
	bool CSHIFT;
	bool LCNTRL;
	for (size_t pp = 0; pp <= cliptxt.size(); pp++) {
		sc = 0;
		CSHIFT = FALSE;
		LCNTRL = FALSE;
		letter = cliptxt[pp];
		switch (letter)
		{
		case '@': sc = 0x03; CSHIFT = TRUE; break;
		case 'A': sc = 0x1E; CSHIFT = TRUE; break;
		case 'B': sc = 0x30; CSHIFT = TRUE; break;
		case 'C': sc = 0x2E; CSHIFT = TRUE; break;
		case 'D': sc = 0x20; CSHIFT = TRUE; break;
		case 'E': sc = 0x12; CSHIFT = TRUE; break;
		case 'F': sc = 0x21; CSHIFT = TRUE; break;
		case 'G': sc = 0x22; CSHIFT = TRUE; break;
		case 'H': sc = 0x23; CSHIFT = TRUE; break;
		case 'I': sc = 0x17; CSHIFT = TRUE; break;
		case 'J': sc = 0x24; CSHIFT = TRUE; break;
		case 'K': sc = 0x25; CSHIFT = TRUE; break;
		case 'L': sc = 0x26; CSHIFT = TRUE; break;
		case 'M': sc = 0x32; CSHIFT = TRUE; break;
		case 'N': sc = 0x31; CSHIFT = TRUE; break;
		case 'O': sc = 0x18; CSHIFT = TRUE; break;
		case 'P': sc = 0x19; CSHIFT = TRUE; break;
		case 'Q': sc = 0x10; CSHIFT = TRUE; break;
		case 'R': sc = 0x13; CSHIFT = TRUE; break;
		case 'S': sc = 0x1F; CSHIFT = TRUE; break;
		case 'T': sc = 0x14; CSHIFT = TRUE; break;
		case 'U': sc = 0x16; CSHIFT = TRUE; break;
		case 'V': sc = 0x2F; CSHIFT = TRUE; break;
		case 'W': sc = 0x11; CSHIFT = TRUE; break;
		case 'X': sc = 0x2D; CSHIFT = TRUE; break;
		case 'Y': sc = 0x15; CSHIFT = TRUE; break;
		case 'Z': sc = 0x2C; CSHIFT = TRUE; break;
		case ' ': sc = 0x39; break;
		case 'a': sc = 0x1E; break;
		case 'b': sc = 0x30; break;
		case 'c': sc = 0x2E; break;
		case 'd': sc = 0x20; break;
		case 'e': sc = 0x12; break;
		case 'f': sc = 0x21; break;
		case 'g': sc = 0x22; break;
		case 'h': sc = 0x23; break;
		case 'i': sc = 0x17; break;
		case 'j': sc = 0x24; break;
		case 'k': sc = 0x25; break;
		case 'l': sc = 0x26; break;
		case 'm': sc = 0x32; break;
		case 'n': sc = 0x31; break;
		case 'o': sc = 0x18; break;
		case 'p': sc = 0x19; break;
		case 'q': sc = 0x10; break;
		case 'r': sc = 0x13; break;
		case 's': sc = 0x1F; break;
		case 't': sc = 0x14; break;
		case 'u': sc = 0x16; break;
		case 'v': sc = 0x2F; break;
		case 'w': sc = 0x11; break;
		case 'x': sc = 0x2D; break;
		case 'y': sc = 0x15; break;
		case 'z': sc = 0x2C; break;
		case '0': sc = 0x0B; break;
		case '1': sc = 0x02; break;
		case '2': sc = 0x03; break;
		case '3': sc = 0x04; break;
		case '4': sc = 0x05; break;
		case '5': sc = 0x06; break;
		case '6': sc = 0x07; break;
		case '7': sc = 0x08; break;
		case '8': sc = 0x09; break;
		case '9': sc = 0x0A; break;
		case '!': sc = 0x02; CSHIFT = TRUE; break;
		case '#': sc = 0x04; CSHIFT = TRUE;	break;
		case '$': sc = 0x05; CSHIFT = TRUE;	break;
		case '%': sc = 0x06; CSHIFT = TRUE;	break;
		case '^': sc = 0x07; CSHIFT = TRUE;	break;
		case '&': sc = 0x08; CSHIFT = TRUE;	break;
		case '*': sc = 0x09; CSHIFT = TRUE;	break;
		case '(': sc = 0x0A; CSHIFT = TRUE;	break;
		case ')': sc = 0x0B; CSHIFT = TRUE;	break;
		case '-': sc = 0x0C; break;
		case '=': sc = 0x0D; break;
		case ';': sc = 0x27; break;
		case '\'': sc = 0x28; break;
		case '/': sc = 0x35; break;
		case '.': sc = 0x34; break;
		case ',': sc = 0x33; break;
		case '\n': sc = 0x1C; break;
		case '+': sc = 0x0D; CSHIFT = TRUE;	break;
		case ':': sc = 0x27; CSHIFT = TRUE;	break;
		case '\"': sc = 0x28; CSHIFT = TRUE; break;
		case '?': sc = 0x35; CSHIFT = TRUE; break;
		case '<': sc = 0x33; CSHIFT = TRUE; break;
		case '>': sc = 0x34; CSHIFT = TRUE; break;
		case '[': sc = 0x1A; LCNTRL = TRUE; break;
		case ']': sc = 0x1B; LCNTRL = TRUE; break;
		case '{': sc = 0x1A; CSHIFT = TRUE; break;
		case '}': sc = 0x1B; CSHIFT = TRUE; break;
		case '\\': sc = 0x2B; LCNTRL = TRUE; break;
		case '|': sc = 0x2B; CSHIFT = TRUE; break;
		case '`': sc = 0x29; break;
		case '~': sc = 0x29; CSHIFT = TRUE; break;
		case '_': sc = 0x0C; CSHIFT = TRUE; break;
		case 0x09: sc = 0x39; break; // TAB
		default: sc = -1; break;
		}
		if (CSHIFT) { out += 0x36; CSHIFT = FALSE; }
		if (LCNTRL) { out += 0x1D; LCNTRL = FALSE; }
		out += sc;
	}
	return out;
}

std::string GetClipboardText()
{
	if (!OpenClipboard(nullptr)) { MessageBox(nullptr, "Unable to open clipboard.", "Clipboard", 0); return {}; }
	HANDLE hClip = GetClipboardData(CF_TEXT);
	if (hClip == nullptr) { CloseClipboard(); MessageBox(nullptr, "No text found in clipboard.", "Clipboard", 0); return {}; }
	const char* tmp = static_cast<char*>(GlobalLock(hClip));
	if (tmp == nullptr) {
		CloseClipboard();  MessageBox(nullptr, "NULL Pointer", "Clipboard", 0); return {};
	}
	std::string out(tmp);
	GlobalUnlock(hClip);
	CloseClipboard();

	return out;
}

bool SetClipboard(const string& sendout) {
	const char* clipout = sendout.c_str();
	const size_t len = strlen(clipout) + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), clipout, len);
	GlobalUnlock(hMem);
	OpenClipboard(nullptr);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
	return TRUE;
}

void CopyText() {
	int idx;
	int tmp;
	int lines;
	int offset;
	int lastchar;
	int BytesPerRow = GetBytesPerRow();
	int GraphicsMode = GetGraphicsMode();
	unsigned int screenstart = GetStartOfVidram();
	if (GraphicsMode != 0) { 
		MessageBox(nullptr, "ERROR: Graphics screen can not be copied.\nCopy can ONLY use a hardware text screen.", "Clipboard", 0); 
		return;
	}
	string out;
	string tmpline;
	if (BytesPerRow == 32) { lines = 15; }
	else { lines = 23; }

	string dbug = "StartofVidram is: " + to_string(screenstart) + "\nGraphicsMode is: " + to_string(GraphicsMode)+"\n";
	OutputDebugString(dbug.c_str());
	
	// Read the lo-res text screen...
	if (BytesPerRow == 32) {
		offset = 0;
		char pcchars[] =
		{
			'@','a','b','c','d','e','f','g',
			'h','i','j','k','l','m','n','o',
			'p','q','r','s','t','u','v','w',
			'x','y','z','[','\\',']',' ',' ',
			' ','!','\"','#','$','%','&','\'',
			'(',')','*','+',',','-','.','/',
			'0','1','2','3','4','5','6','7',
			'8','9',':',';','<','=','>','?',
			'@','A','B','C','D','E','F','G',
			'H','I','J','K','L','M','N','O',
			'P','Q','R','S','T','U','V','W',
			'X','Y','Z','[','\\',']',' ',' ',
			' ','!','\"','#','$','%','&','\'',
			'(',')','*','+',',','-','.','/',
			'0','1','2','3','4','5','6','7',
			'8','9',':',';','<','=','>','?',
			'@','a','b','c','d','e','f','g',
			'h','i','j','k','l','m','n','o',
			'p','q','r','s','t','u','v','w',
			'x','y','z','[','\\',']',' ',' ',
			' ','!','\"','#','$','%','&','\'',
			'(',')','*','+',',','-','.','/',
			'0','1','2','3','4','5','6','7',
			'8','9',':',';','<','=','>','?'
		};

		for (int y = 0; y <= lines; y++) {
			lastchar = 0;
			tmpline.clear();
			tmp = 0;
			for (idx = 0; idx < BytesPerRow; idx++) {
				tmp = MemRead8(0x0400 + y * BytesPerRow + idx);
				if (tmp == 32 || tmp == 64 || tmp == 96) { tmp = 30 + offset; } 
				else { lastchar = idx + 1; }
				tmpline += pcchars[tmp - offset]; 
			}
			tmpline = tmpline.substr(0, lastchar);
			if (lastchar != 0) { out += tmpline; out += "\n"; }

		}
		if (out == "") { MessageBox(nullptr, "No text found on screen.", "Clipboard", 0); }
	}
	else if (BytesPerRow == 40 || BytesPerRow == 80) {
		offset = 32;
		int pcchars[] =
		{
			' ','!','\"','#','$','%','&','\'',
			'(',')','*','+',',','-','.','/',
			'0','1','2','3','4','5','6','7',
			'8','9',':',';','<','=','>','?',
			'@','A','B','C','D','E','F','G',
			'H','I','J','K','L','M','N','O',
			'P','Q','R','S','T','U','V','W',
			'X','Y','Z','[','\\',']',' ',' ',
			'^','a','b','c','d','e','f','g',
			'h','i','j','k','l','m','n','o',
			'p','q','r','s','t','u','v','w',
			'x','y','z','{','|','}','~','_',
			'Ç','ü','é','â','ä','à','å','ç',
			'ê','ë','è','ï','î','ß','Ä','Â',
			'Ó','æ','Æ','ô','ö','ø','û','ù',
			'Ø','Ö','Ü','§','£','±','º','',
			' ',' ','!','\"','#','$','%','&',
			'\'','(',')','*','+',',','-','.',
			'/','0','1','2','3','4','5','6',
			'7','8','9',':',';','<','=','>',
			'?','@','A','B','C','D','E','F',
			'G','H','I','J','K','L','M','N',
			'O','P','Q','R','S','T','U','V',
			'W','X','Y','Z','[','\\',']',' ',
			' ','^','a','b','c','d','e','f',
			'g','h','i','j','k','l','m','n',
			'o','p','q','r','s','t','u','v',
			'w','x','y','z','{','|','}','~','_'
		};

		for (int y = 0; y <= lines; y++) {
			lastchar = 0;
			tmpline.clear();
			tmp = 0;
			for (idx = 0; idx < BytesPerRow * 2; idx += 2) {
				tmp = GetMem(screenstart + y * (BytesPerRow * 2) + idx);
				if (tmp == 32 || tmp == 64 || tmp == 96) { tmp = offset; }
				else { lastchar = idx / 2 + 1; }
				tmpline += pcchars[tmp - offset];
			}
			tmpline = tmpline.substr(0, lastchar);
			if (lastchar != 0) { out += tmpline; out += "\n"; }
		}
	}
	
	bool succ = SetClipboard(out);
}

void PasteBASIC() {
	codepaste = true;
	PasteText();
	codepaste = false;
}
void PasteBASICWithNew() {
	int tmp=MessageBox(nullptr, "Warning: This operation will erase the Coco's BASIC memory\nbefore pasting. Continue?", "Clipboard", MB_YESNO);
	if (tmp != 6) { return; }
	codepaste = true;
	PasteWithNew = true;
	PasteText();
	codepaste = false;
	PasteWithNew = false;
}

