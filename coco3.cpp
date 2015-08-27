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
void AudioOut(void);
void CassOut(void);
void CassIn(void);
void (*AudioEvent)(void)=AudioOut;
void SetMasterTickCounter(void);
void (*DrawTopBoarder[4]) (SystemState *)={DrawTopBoarder8,DrawTopBoarder16,DrawTopBoarder24,DrawTopBoarder32};
void (*DrawBottomBoarder[4]) (SystemState *)={DrawBottomBoarder8,DrawBottomBoarder16,DrawBottomBoarder24,DrawBottomBoarder32};
void (*UpdateScreen[4]) (SystemState *)={UpdateScreen8,UpdateScreen16,UpdateScreen24,UpdateScreen32};
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




