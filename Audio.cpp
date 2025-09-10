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

//#include <dplay.h>
#include <dsound.h>
#include <stdio.h>
#include "defines.h"
#include "Vcc.h"
#include "config.h"
#include "coco3.h"
#include "audio.h"
#include "logger.h"
#include "BuildConfig.h"
#include "Cassette.h"

#if USE_DEBUG_AUDIOTAPE
#include "IDisplayDebug.h"
#endif

using AuxBufferType = VCC::Array<VCC::Array<uint32_t, AUDIO_RATE / 60>, 6>;

constexpr auto MAXCARDS = 12u;
//PlayBack
static LPDIRECTSOUND	lpds;           // directsound interface pointer
static DSBUFFERDESC		dsbd;           // directsound description
static DSCAPS			dscaps;         // directsound caps
static DSBCAPS			dsbcaps;        // directsound buffer caps
//Record
static LPDIRECTSOUNDCAPTURE8	lpdsin;
static DSCBUFFERDESC			dsbdin;           // directsound description

HRESULT hr;
static LPDIRECTSOUNDBUFFER	lpdsbuffer1=nullptr;			//the sound buffers
static LPDIRECTSOUNDCAPTUREBUFFER	lpdsbuffer2=nullptr;	//the sound buffers for capture
static WAVEFORMATEX pcmwf;								//generic waveformat structure
static void *SndPointer1 = nullptr,*SndPointer2 = nullptr;
static unsigned int BitRate=0;
static DWORD SndLenth1= 0,SndLenth2= 0,SndBuffLenth = 0;
static unsigned char InitPassed=0;
static unsigned int BlockSize=0;
static AuxBufferType AuxBuffer;
static DWORD WritePointer=0;
static DWORD BuffOffset=0;
static char AuxBufferPointer=0;
static int CardCount=0;
static unsigned int CurrentRate=0;
static unsigned char AudioPause=0;
static SndCardList *Cards=nullptr;
BOOL CALLBACK DSEnumCallback(LPGUID,LPCSTR,LPCSTR,LPVOID);


#if USE_DEBUG_AUDIOTAPE
static void* AudioBaseAddress=0;
const int AudioHistorySize = 50;
struct AudioBufferHistory
{
	void* aBuf;
	unsigned int aSize;
	void* bBuf;
	unsigned int bSize;
	unsigned int playPos;
	unsigned int writePos;
	unsigned int freeBlocks;
};
AudioBufferHistory gAudioBufferHistory[AudioHistorySize];
#endif

int SoundInit (HWND main_window_handle,const _GUID * Guid,unsigned int Rate)
{
	Rate=(Rate & 3);
	if (Rate != 0)	//Force 44100 or Mute
		Rate = 3;
	CurrentRate=Rate;
	if (InitPassed)
	{
		PauseAudio(true);
		ResetAudio();
		SoundDeInit();
	}
	SndLenth1= 0;
	SndLenth2= 0;
	BuffOffset=0;
	AuxBufferPointer=0;
	BitRate=iRateList[Rate];
	BlockSize=BitRate*4/TARGETFRAMERATE;
	SndBuffLenth = (BlockSize*AUDIOBUFFERS);
	if (Rate)
	{
		hr=DirectSoundCreate(Guid, &lpds, nullptr);	// create a directsound object
		if (hr!=DS_OK)
			return 1;

		hr=lpds->SetCooperativeLevel(main_window_handle,DSSCL_NORMAL);	// set cooperation level normal DSSCL_EXCLUSIVE
		if (hr!=DS_OK)
			return 1;
		// set up the format data structure
		memset(&pcmwf, 0, sizeof(WAVEFORMATEX));
		pcmwf.wFormatTag	  = WAVE_FORMAT_PCM;
		pcmwf.nChannels		  = 2;
		pcmwf.nSamplesPerSec  = BitRate;
		pcmwf.wBitsPerSample  = 16;
		pcmwf.nBlockAlign	  = (pcmwf.wBitsPerSample * pcmwf.nChannels) >> 3;
		pcmwf.nAvgBytesPerSec = pcmwf.nSamplesPerSec * pcmwf.nBlockAlign;
		pcmwf.cbSize		  = 0;

		// create the secondary buffer 
		memset(&dsbd,0,sizeof(DSBUFFERDESC));
		dsbd.dwSize			= sizeof(DSBUFFERDESC);
		dsbd.dwFlags		= DSBCAPS_GETCURRENTPOSITION2|DSBCAPS_LOCSOFTWARE|DSBCAPS_STATIC|DSBCAPS_GLOBALFOCUS;
		dsbd.dwBufferBytes	= SndBuffLenth;
		dsbd.lpwfxFormat	= &pcmwf;

		hr=lpds->CreateSoundBuffer(&dsbd,&lpdsbuffer1,nullptr);
		if (hr!=DS_OK)
			return 1;

		// Clear out sound buffers
		hr= lpdsbuffer1->Lock(0,SndBuffLenth,&SndPointer1,&SndLenth1,&SndPointer2,&SndLenth2,DSBLOCK_ENTIREBUFFER );
		if (hr!=DS_OK)
			return 1;

#if USE_DEBUG_AUDIOTAPE
		AudioBaseAddress = SndPointer1;
		memset(gAudioBufferHistory, 0, sizeof(gAudioBufferHistory));
#endif
		memset (SndPointer1,0,SndBuffLenth);
		hr=lpdsbuffer1->Unlock(SndPointer1,SndLenth1,SndPointer2,SndLenth2); 
		if (hr!=DS_OK)
			return 1;

		
		lpdsbuffer1->SetCurrentPosition(0);
		hr=lpdsbuffer1->Play(0,0,DSBPLAY_LOOPING );	// play the sound in looping mode
		if (hr!=DS_OK)
			return 1;
		InitPassed=1;
		AudioPause=0;
	}
	SetAudioRate(AUDIO_RATE);
	return 0;
}

void FlushAudioBuffer(unsigned int *Abuffer,unsigned int Lenth)
{
	const unsigned char *Abuffer2=(unsigned char *)Abuffer;

	if (!InitPassed || AudioPause || !Abuffer)
		return;

	auto freeBlocks = GetFreeBlockCount();
	if (freeBlocks<=0) // should only happen when frame skipping or unthrottled
		return;

	// Levels calculated in config.c if Audio config dialog is running
	UpdateSoundBar(Abuffer,Lenth);

#if USE_DEBUG_AUDIOTAPE
	memcpy(&gAudioBufferHistory[0], &gAudioBufferHistory[1], sizeof(gAudioBufferHistory) - sizeof(*gAudioBufferHistory));
	unsigned long WriteCursor = 0, PlayCursor = 0;
	lpdsbuffer1->GetCurrentPosition(&PlayCursor, &WriteCursor);
	auto& history = gAudioBufferHistory[AudioHistorySize - 1];
	history.playPos = PlayCursor;
	history.writePos = WriteCursor;
	history.freeBlocks = freeBlocks;
#endif

	hr=lpdsbuffer1->Lock(BuffOffset,Lenth,&SndPointer1,&SndLenth1,&SndPointer2,&SndLenth2,0);
	if (hr != DS_OK)
		return;

	memcpy(SndPointer1, Abuffer2, SndLenth1);       // copy first section of circular buffer
	if (SndPointer2 != nullptr)                        // copy last section if wrapped
		memcpy(SndPointer2, Abuffer2 + SndLenth1, SndLenth2);
	hr=lpdsbuffer1->Unlock(SndPointer1,SndLenth1,SndPointer2,SndLenth2);// unlock the buffer
	BuffOffset=(BuffOffset + Lenth)% SndBuffLenth;  //Where to write next

#if USE_DEBUG_AUDIOTAPE
	history.aBuf = SndPointer1;
	history.aSize = SndLenth1;
	history.bBuf = SndPointer2;
	history.bSize = SndLenth2;
	const int s = 75;
	const int xp = 1000;
	for (int i = 0; i < 50;++i)
	{
		auto& data = gAudioBufferHistory[i];
		VCC::Pixel col(VCC::ColorYellow); col.a = 200;
		int x = (char*)data.aBuf - (char*)AudioBaseAddress;
		int y = 400 + i * 8;
		DebugDrawBox(xp + x / s, y, data.aSize / s, 3, col);
		if (data.bBuf)
		{
			VCC::Pixel col(VCC::ColorGreen); col.a = 200;
			int x1 = (char*)data.bBuf - (char*)AudioBaseAddress;
			DebugDrawBox(xp + x1 / s, y, data.bSize / s, 3, col);
		}
		DebugDrawLine(xp + data.playPos / s, y - 1, xp + data.playPos / s, y + 5, VCC::ColorGreen);
		DebugDrawLine(xp + data.writePos / s, y - 1, xp + data.writePos / s, y + 5, VCC::ColorRed);
		DebugDrawBox(xp - 20, y, data.freeBlocks, 3, col);
	}
#endif
}


int GetFreeBlockCount(void) //return 0 on full buffer
 {
	unsigned long WriteCursor=0,PlayCursor=0;
	long RetVal=0,MaxSize=0;
	if ((!InitPassed) | AudioPause)
		return AUDIOBUFFERS;
	RetVal=lpdsbuffer1->GetCurrentPosition(&PlayCursor,&WriteCursor);
	if (BuffOffset <=PlayCursor)
		MaxSize= PlayCursor - BuffOffset;
	else
		MaxSize= SndBuffLenth - BuffOffset + PlayCursor;
	return(MaxSize/BlockSize); 

 }

 void PurgeAuxBuffer(void)
 {
	if ((!InitPassed) | AudioPause)
		return;
	return;
	AuxBufferPointer--;			//Normally points to next free block Point to last used block
	if (AuxBufferPointer>=0)	//zero is a valid data block
	{
		while (GetFreeBlockCount() <= 0);


		hr=lpdsbuffer1->Lock(BuffOffset,BlockSize,&SndPointer1,&SndLenth1,&SndPointer2,&SndLenth2,0);//DSBLOCK_FROMWRITECURSOR);
		if (hr != DS_OK)
			return;
		AuxBuffer[AuxBufferPointer].MemCpyTo(SndPointer1, SndLenth1);
		if (SndPointer2 != nullptr)
			AuxBuffer[AuxBufferPointer].MemCpyTo((SndLenth1 >> 2), SndPointer2, SndLenth2);
		BuffOffset=(BuffOffset + BlockSize)% SndBuffLenth;
		hr=lpdsbuffer1->Unlock(SndPointer1,SndLenth1,SndPointer2,SndLenth2);
		AuxBufferPointer--;
	}
	AuxBufferPointer=0;
	return;
}

int GetSoundCardList (SndCardList *List)
{
	CardCount=0;
	Cards=List;
	DirectSoundEnumerate(DSEnumCallback, nullptr); 
	return CardCount;
}

BOOL CALLBACK DSEnumCallback(LPGUID lpGuid,LPCSTR lpcstrDescription,LPCSTR /*lpcstrModule*/,LPVOID /*lpContext*/)          
{
	strncpy(Cards[CardCount].CardName,lpcstrDescription,63);
	Cards[CardCount++].Guid=lpGuid;
	return (CardCount<MAXCARDS);
}

int SoundDeInit(void)
{
	if (InitPassed)
	{
		InitPassed=0;
		if (lpdsbuffer1 != nullptr)
		{
			hr = lpdsbuffer1->Release();
			lpdsbuffer1 = nullptr;
		}

		if (lpds != nullptr)
		{
			hr = lpds->Release();
			lpds = nullptr;
		}
	}
	return 0;
}

int SoundInInit (const _GUID * Guid)
{
	hr=DirectSoundCaptureCreate(Guid, &lpdsin, nullptr);
	if (hr!=DS_OK)
		return 1;
	dsbdin.dwSize=sizeof(DSCBUFFERDESC); // Size of the structure
	dsbdin.dwFlags=0;
	dsbdin.dwReserved=0;
	dsbdin.lpwfxFormat=&pcmwf; //fix
	dsbdin.dwBufferBytes=SndBuffLenth;
	hr=lpdsin->CreateCaptureBuffer(&dsbdin, &lpdsbuffer2, nullptr);
	if (hr!=DS_OK)
		return 1;
	lpdsbuffer2->Start(hr);
	return 0;
}

unsigned int GetSoundStatus(void)
{
	return CurrentRate;
}

void ResetAudio (void)
{
	SetAudioRate(GetTapeRate());
	if (InitPassed)
		lpdsbuffer1->SetCurrentPosition(0);
	BuffOffset=0;
	AuxBufferPointer=0;
	return;
}

unsigned char PauseAudio(unsigned char Pause)
{
	AudioPause=Pause;
	if (InitPassed)
	{
		if (AudioPause==1)
			hr=lpdsbuffer1->Stop();
		else
			hr=lpdsbuffer1->Play(0,0,DSBPLAY_LOOPING);
	}
	return AudioPause;
}

