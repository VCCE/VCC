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

#include "windows.h"
#include "stdio.h"
#include "ddraw.h"
#include <math.h>
#include "defines.h"
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
#include "cassette.h"
#include "DirectDrawInterface.h"
#include "logger.h"
#include "keyboard.h"
#include <string>
#include <iostream>
#include "config.h"
#include "tcc1014mmu.h"

//int CPUExeca(int);

//****************************************
	static double SoundInterupt=0;
	static double PicosToSoundSample=SoundInterupt;
	static double CyclesPerSecord=(COLORBURST/4)*(TARGETFRAMERATE/FRAMESPERSECORD);
	static double LinesPerSecond= TARGETFRAMERATE * LINESPERSCREEN;
	static double PicosPerLine = PICOSECOND / LinesPerSecond;
	static double CyclesPerLine = CyclesPerSecord / LinesPerSecond;
	static double CycleDrift=0;
	static double CyclesThisLine=0;
	static unsigned int StateSwitch=0;
	unsigned short SoundRate=0;
//*****************************************************

static unsigned char HorzInteruptEnabled=0,VertInteruptEnabled=0;
static unsigned char TopBoarder=0,BottomBoarder=0;
static unsigned char LinesperScreen;
static unsigned char TimerInteruptEnabled=0;
static int MasterTimer=0; 
static unsigned short TimerClockRate=0;
static int TimerCycleCount=0;
static double MasterTickCounter=0,UnxlatedTickCounter=0,OldMaster=0;
static double PicosThisLine=0;
static unsigned char BlinkPhase=1;
static unsigned int AudioBuffer[16384];
static unsigned char CassBuffer[8192];
static unsigned short AudioIndex=0;
double PicosToInterupt=0;
static int IntEnable=0;
static int SndEnable=1;
static int OverClock=1;
static unsigned char SoundOutputMode=0;	//Default to Speaker 1= Cassette

static int clipcycle = 1, cyclewait=2000;
bool codepaste, PasteWithNew = false; 
char tmpthrottle = 0;
int CurrentKeyMap;
void AudioOut(void);
void CassOut(void);
void CassIn(void);
void (*AudioEvent)(void)=AudioOut;
void SetMasterTickCounter(void);
void (*DrawTopBoarder[4]) (SystemState *)={DrawTopBoarder8,DrawTopBoarder16,DrawTopBoarder24,DrawTopBoarder32};
void (*DrawBottomBoarder[4]) (SystemState *)={DrawBottomBoarder8,DrawBottomBoarder16,DrawBottomBoarder24,DrawBottomBoarder32};
void (*UpdateScreen[4]) (SystemState *)={UpdateScreen8,UpdateScreen16,UpdateScreen24,UpdateScreen32};
std::string GetClipboardText();

using namespace std;
string clipboard;
STRConfig ClipConfig;

_inline int CPUCycle(void);
float RenderFrame (SystemState *RFState)
{
	static unsigned short FrameCounter=0;

//********************************Start of frame Render*****************************************************
	SetBlinkState(BlinkPhase);
	irq_fs(0);				//FS low to High transition start of display Boink needs this

	for (RFState->LineCounter=0;RFState->LineCounter<13;RFState->LineCounter++)		//Vertical Blanking 13 H lines 
		CPUCycle();

	for (RFState->LineCounter=0;RFState->LineCounter<4;RFState->LineCounter++)		//4 non-Rendered top Boarder lines
		CPUCycle();

	if (!(FrameCounter % RFState->FrameSkip))
		if (LockScreen(RFState))
			return(0);

	for (RFState->LineCounter=0;RFState->LineCounter<(TopBoarder-4);RFState->LineCounter++) 		
	{
		if (!(FrameCounter % RFState->FrameSkip))
			DrawTopBoarder[RFState->BitDepth](RFState);
		CPUCycle();
	}

	for (RFState->LineCounter=0;RFState->LineCounter<LinesperScreen;RFState->LineCounter++)		//Active Display area		
	{
		CPUCycle();
		if (!(FrameCounter % RFState->FrameSkip))
			UpdateScreen[RFState->BitDepth] (RFState);
	} 
	irq_fs(1);  //End of active display FS goes High to Low
	if (VertInteruptEnabled)
		GimeAssertVertInterupt();	
	for (RFState->LineCounter=0;RFState->LineCounter < (BottomBoarder) ;RFState->LineCounter++)	// Bottom boarder 
	{
//		if ( (RFState->LineCounter==1) & (VertInteruptEnabled) )	//Vert Interupt occurs 1 line into 
//			GimeAssertVertInterupt();								// Bottom Boarder MPATDEMO
		CPUCycle();
		if (!(FrameCounter % RFState->FrameSkip))
			DrawBottomBoarder[RFState->BitDepth](RFState);
	}

	if (!(FrameCounter % RFState->FrameSkip))
	{
		UnlockScreen(RFState);
		SetBoarderChange(0);
	}

	for (RFState->LineCounter=0;RFState->LineCounter<6;RFState->LineCounter++)		//Vertical Retrace 6 H lines
		CPUCycle();

	switch (SoundOutputMode)
	{
	case 0:
		FlushAudioBuffer(AudioBuffer,AudioIndex<<2);
		break;
	case 1:
		FlushCassetteBuffer(CassBuffer,AudioIndex);
		break;
	case 2:
		LoadCassetteBuffer(CassBuffer);

		break;
	}
	AudioIndex=0;


/*
	//Debug Code
	Frames++;
	if (Frames==60)
	{
		Frames=0;
		sprintf(Msga,"Total Cycles = %i Scan lines = %i LPS= %i\n",TotalCycles,Scans,LinesperScreen+TopBoarder+BottomBoarder+19);
		WriteLog(Msga,0);
		TotalCycles=0;
		Scans=0;
	}
*/
	return(CalculateFPS());
}

void SetClockSpeed(unsigned short Cycles)
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
	BottomBoarder= 243 - (TopBoarder + LinesperScreen); //4 lines of top boarder are unrendered 244-4=240 rendered scanlines
	return;
}


_inline int CPUCycle(void)	
{
	if (HorzInteruptEnabled)
		GimeAssertHorzInterupt();
	irq_hs(ANY);
	PakTimer();
	PicosThisLine+=PicosPerLine;	
	while (PicosThisLine>1)		
	{
		StateSwitch=0;
		if ((PicosToInterupt<=PicosThisLine) & IntEnable )	//Does this iteration need to Timer Interupt
			StateSwitch=1;
		if ((PicosToSoundSample<=PicosThisLine) & SndEnable)//Does it need to collect an Audio sample
			StateSwitch+=2;
		switch (StateSwitch)
		{
			case 0:		//No interupts this line
				CyclesThisLine= CycleDrift + (PicosThisLine * CyclesPerLine * OverClock/PicosPerLine);
				if (CyclesThisLine>=1)	//Avoid un-needed CPU engine calls
					CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
				else 
					CycleDrift=CyclesThisLine;
				PicosToInterupt-=PicosThisLine;
				PicosToSoundSample-=PicosThisLine;
				PicosThisLine=0;
			break;

			case 1:		//Only Interupting
				PicosThisLine-=PicosToInterupt;
				CyclesThisLine= CycleDrift + (PicosToInterupt * CyclesPerLine * OverClock/PicosPerLine);
				if (CyclesThisLine>=1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
				else 
					CycleDrift=CyclesThisLine;
				GimeAssertTimerInterupt();
				PicosToSoundSample-=PicosToInterupt;
				PicosToInterupt=MasterTickCounter;
			break;

			case 2:		//Only Sampling
				PicosThisLine-=PicosToSoundSample;
				CyclesThisLine=CycleDrift +(PicosToSoundSample * CyclesPerLine * OverClock/PicosPerLine );
				if (CyclesThisLine>=1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
				else 
					CycleDrift=CyclesThisLine;
				AudioEvent();
				PicosToInterupt-=PicosToSoundSample;
				PicosToSoundSample=SoundInterupt;
			break;

			case 3:		//Interupting and Sampling
				if (PicosToSoundSample<PicosToInterupt)
				{
					PicosThisLine-=PicosToSoundSample;	
					CyclesThisLine=CycleDrift +(PicosToSoundSample * CyclesPerLine * OverClock/PicosPerLine);
					if (CyclesThisLine>=1)
						CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
					else 
						CycleDrift=CyclesThisLine;
					AudioEvent();
					PicosToInterupt-=PicosToSoundSample;	
					PicosToSoundSample=SoundInterupt;
					PicosThisLine-=PicosToInterupt;

					CyclesThisLine= CycleDrift +(PicosToInterupt * CyclesPerLine * OverClock/PicosPerLine);
					if (CyclesThisLine>=1)
						CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
					else 
						CycleDrift=CyclesThisLine;
					GimeAssertTimerInterupt();
					PicosToSoundSample-=PicosToInterupt;
					PicosToInterupt=MasterTickCounter;
					break;
				}

				if (PicosToSoundSample>PicosToInterupt)
				{
					PicosThisLine-=PicosToInterupt;
					CyclesThisLine= CycleDrift +(PicosToInterupt * CyclesPerLine * OverClock/PicosPerLine);
					if (CyclesThisLine>=1)
						CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
					else 
						CycleDrift=CyclesThisLine;
					GimeAssertTimerInterupt();		
					PicosToSoundSample-=PicosToInterupt;
					PicosToInterupt=MasterTickCounter;
					PicosThisLine-=PicosToSoundSample;
					CyclesThisLine=  CycleDrift +(PicosToSoundSample * CyclesPerLine * OverClock/PicosPerLine);	
					if (CyclesThisLine>=1)
						CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
					else 
						CycleDrift=CyclesThisLine;
					AudioEvent();
					PicosToInterupt-=PicosToSoundSample;
					PicosToSoundSample=SoundInterupt;
					break;
				}
					//They are the same (rare)
			PicosThisLine-=PicosToInterupt;
			CyclesThisLine=CycleDrift +(PicosToSoundSample * CyclesPerLine * OverClock/PicosPerLine );
			if (CyclesThisLine>1)
				CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
			else 
				CycleDrift=CyclesThisLine;
			GimeAssertTimerInterupt();
			AudioEvent();
			PicosToInterupt=MasterTickCounter;
			PicosToSoundSample=SoundInterupt;
		}
	}
	if (!clipboard.empty()) {
		char tmp[] = { 0x00 };
		char kbstate = 2;
		int z = 0;

		//Remember the original throttle setting.
		//Set it to off. We need speed for this!
		if (tmpthrottle == 0) {
			tmpthrottle = SetSpeedThrottle(QUERY);
			if (tmpthrottle == 0) { tmpthrottle = 2; } // 2 = No throttle.
		}
		SetSpeedThrottle(0);

		strcpy(tmp, clipboard.substr(0, 1).c_str());
		if (clipcycle == 1) {
			if (tmp[z] == 0x36) {
				vccKeyboardHandleKey(0x36, 0x36, kEventKeyDown);  //Press shift and...
				clipboard = clipboard.substr(1, clipboard.length() - 1); // get the next key in the string
				strcpy(tmp, clipboard.substr(0, 1).c_str());
				vccKeyboardHandleKey(tmp[z], tmp[z], kEventKeyDown);
				if (tmp[z] == 0x1c) {
					cyclewait = 6000; 
				}
				else { cyclewait = 2000; }


			}
			else { vccKeyboardHandleKey(tmp[z], tmp[z], kEventKeyDown);
			if (tmp[z] == 0x1c) { cyclewait = 6000; 
			}
				else { cyclewait = 2000; }
			}

			clipboard = clipboard.substr(1, clipboard.length() - 1);
			if (clipboard.empty()) {
				SetPaste(false);
				//Done pasting. Reset throttle to original state
				if (tmpthrottle == 2) { SetSpeedThrottle(0); }
				else { SetSpeedThrottle(1); }
				//...and reset the keymap to the original state

				vccKeyboardBuildRuntimeTable((keyboardlayout_e)CurrentKeyMap);
				tmpthrottle = 0;
			}
		}
		else if (clipcycle == 500) {
			vccKeyboardHandleKey(0x36, 0x36, kEventKeyUp);
			vccKeyboardHandleKey(0x42, tmp[z], kEventKeyUp);

			if (!GetPaste()) { 
				clipboard.clear(); 
				SetPaste(false); 
				if (tmpthrottle == 2) { SetSpeedThrottle(0); }
				else { SetSpeedThrottle(1); }
			}
		}
		clipcycle++; if (clipcycle > cyclewait) { clipcycle = 1; }

	}
return(0);
}

void SetTimerInteruptState(unsigned char State)
{
	TimerInteruptEnabled=State;
	return;
}

void SetInteruptTimer(unsigned short Timer)
{
	UnxlatedTickCounter=(Timer & 0xFFF);
	SetMasterTickCounter();
	return;
}

void SetTimerClockRate (unsigned char Tmp)	//1= 279.265nS (1/ColorBurst) 
{											//0= 63.695uS  (1/60*262)  1 scanline time
	TimerClockRate=!!Tmp;
	SetMasterTickCounter();
	return;
}

void SetMasterTickCounter(void)
{
//	double Rate[2]={63613.2315,279.265};
//	double Rate[2]={279.265*228,279.265};
	double Rate[2]={PICOSECOND/(TARGETFRAMERATE*LINESPERSCREEN),PICOSECOND/COLORBURST};
	if (UnxlatedTickCounter==0)
		MasterTickCounter=0;
	else
		MasterTickCounter=(UnxlatedTickCounter+2)* Rate[TimerClockRate];

	if (MasterTickCounter != OldMaster)  
	{
		OldMaster=MasterTickCounter;
		PicosToInterupt=MasterTickCounter;
	}
	if (MasterTickCounter!=0)
		IntEnable=1;
	else
		IntEnable=0;
	return;
}

void MiscReset(void)
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
	PicosToSoundSample=SoundInterupt;
	CycleDrift=0;
	CyclesThisLine=0;
	PicosThisLine=0;
	IntEnable=0;
	AudioIndex=0;
	ResetAudio();
	return;
}
/*
int CPUExeca(int Loops)
{
	int RetVal=0;
	TotalCycles+=Loops;
	RetVal=CPUExec(Loops);
	TotalCycles+=abs(RetVal);
	return(RetVal);
}
*/
unsigned short SetAudioRate (unsigned short Rate)
{

	SndEnable=1;
	SoundInterupt=0;
	CycleDrift=0;
	AudioIndex=0;
	if (Rate != 0)	//Force Mute or 44100Hz
		Rate = 44100;

	if (Rate==0)
		SndEnable=0;
	else
	{
		SoundInterupt=PICOSECOND/Rate;
		PicosToSoundSample=SoundInterupt;
	}
	SoundRate=Rate;
	return(0);
}

void AudioOut(void)
{

	AudioBuffer[AudioIndex++]=GetDACSample();
	return;
}

void CassOut(void)
{
	CassBuffer[AudioIndex++]=GetCasSample();
	return;
}

void CassIn(void)
{
	AudioBuffer[AudioIndex]=GetDACSample();
	SetCassetteSample(CassBuffer[AudioIndex++]);
	return;
}



unsigned char SetSndOutMode(unsigned char Mode)  //0 = Speaker 1= Cassette Out 2=Cassette In
{
	static unsigned char LastMode=0;
	static unsigned short PrimarySoundRate=SoundRate;

	switch (Mode)
	{
	case 0:
		if (LastMode==1)	//Send the last bits to be encoded
			FlushCassetteBuffer(CassBuffer,AudioIndex);

		AudioEvent=AudioOut;
		SetAudioRate (PrimarySoundRate);
//		SetAudioRate(44100);
		break;

	case 1:
		AudioEvent=CassOut;
		PrimarySoundRate=SoundRate;
		SetAudioRate (TAPEAUDIORATE);
		break;

	case 2:
		AudioEvent=CassIn;
		PrimarySoundRate=SoundRate;;
		SetAudioRate (TAPEAUDIORATE);
		break;

	default:	//QUERY
		return(SoundOutputMode);
		break;
	}

	if (Mode != LastMode)
	{

//		if (LastMode==1)
//			FlushCassetteBuffer(CassBuffer,AudioIndex);	//get the last few bytes
		AudioIndex=0;	//Reset Buffer on true mode switch
		LastMode=Mode;
	}
	SoundOutputMode=Mode;
	return(SoundOutputMode);
}

void PasteText() {
	using namespace std;
	std::string tmp;
	string cliptxt, clipparse, lines, out, debugout;
	char sc;
	char letter;
	bool CSHIFT;
	bool LCNTRL;
	int GraphicsMode = GetGraphicsMode();
	if (GraphicsMode != 0) {
		int tmp = MessageBox(0, "Warning: You are not in text mode. Continue Pasting?", "Clipboard", MB_YESNO);
		if (tmp != 6) { return; }
	}
	SetPaste(true);


	//This sets the keyboard to Natural, 
	//but we need to read it first so we can set it back
	CurrentKeyMap = GetKeyboardLayout();
	vccKeyboardBuildRuntimeTable((keyboardlayout_e)1);
	cliptxt = GetClipboardText().c_str();
	if (PasteWithNew) { cliptxt = "NEW\n" + cliptxt; }
	for (int t = 0; t < cliptxt.length(); t++) {
		char tmp = cliptxt[t];
		if ( tmp != (char)'\n') {
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
				
				MessageBox(0, linestr.c_str(), "Clipboard", 0);
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
	cliptxt = clipparse; 

	for (int pp = 0; pp <= cliptxt.size(); pp++) {
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
		case '\\"': sc = 0x2B; LCNTRL = TRUE; break;
		case '|': sc = 0x2B; CSHIFT = TRUE; break;
		case '`': sc = 0x29; break;
		case '~': sc = 0x29; CSHIFT = TRUE; break;
		case '_': sc = 0x0C; CSHIFT = TRUE; break;
		case 0x09: sc = 0x39; break; // TAB
		default: sc = 0xFF;	break;
		}
		if (CSHIFT) { out += 0x36; CSHIFT = FALSE; }
		if (LCNTRL) { out += 0x1D; LCNTRL = FALSE; }
		out += sc;

	}
	clipboard = out;
}

std::string GetClipboardText()
{
	if (!OpenClipboard(nullptr)) { MessageBox(0, "Unable to open clipboard.", "Clipboard", 0); return(""); }
	HANDLE hClip = GetClipboardData(CF_TEXT);
	if (hClip == nullptr) { CloseClipboard(); MessageBox(0, "No text found in clipboard.", "Clipboard", 0); return(""); }
	char* tmp = static_cast<char*>(GlobalLock(hClip));
	if (tmp == nullptr) {
		CloseClipboard();  MessageBox(0, "NULL Pointer", "Clipboard", 0); return("");
	}
	std::string out(tmp);
	GlobalUnlock(hClip);
	CloseClipboard();

	return out;
}

bool SetClipboard(string sendout) {
	const char* clipout = sendout.c_str();
	const size_t len = strlen(clipout) + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), clipout, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
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
	bool os9;
	int BytesPerRow = GetBytesPerRow();
	int GraphicsMode = GetGraphicsMode();
	unsigned int screenstart = GetStartOfVidram();
	if (GraphicsMode != 0) { 
		MessageBox(0, "ERROR: Graphics screen can not be copied.\nCopy can ONLY use a hardware text screen.", "Clipboard", 0); 
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
		if (out == "") { MessageBox(0, "No text found on screen.", "Clipboard", 0); }
	}
	else if (BytesPerRow == 40 || BytesPerRow == 80) {
		offset = 32;
		char pcchars[] =
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
	
	//if (BytesPerRow == 32) { out = out.substr(0, out.length() - 2); }
	bool succ = SetClipboard(out);
}

void PasteBASIC() {
	codepaste = true;
	PasteText();
	codepaste = false;
}
void PasteBASICWithNew() {
	int tmp=MessageBox(0, "Warning: This operation will erase the Coco's BASIC memory\nbefore pasting. Continue?", "Clipboard", MB_YESNO);
	if (tmp != 6) { return; }
	codepaste = true;
	PasteWithNew = true;
	PasteText();
	codepaste = false;
	PasteWithNew = false;
}

