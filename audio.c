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

#define MAXCARDS	12
//PlayBack
static LPDIRECTSOUND	lpds;           // directsound interface pointer
static DSBUFFERDESC		dsbd;           // directsound description
static DSCAPS			dscaps;         // directsound caps
static DSBCAPS			dsbcaps;        // directsound buffer caps
//Record
static LPDIRECTSOUNDCAPTURE8	lpdsin;
static DSCBUFFERDESC			dsbdin;           // directsound description


HRESULT hr;
static LPDIRECTSOUNDBUFFER	lpdsbuffer1=NULL;			//the sound buffers
static LPDIRECTSOUNDCAPTUREBUFFER	lpdsbuffer2=NULL;	//the sound buffers for capture
static WAVEFORMATEX pcmwf;								//generic waveformat structure
static void *SndPointer1 = NULL,*SndPointer2 = NULL;
static unsigned short BitRate=0;
static DWORD SndLenth1= 0,SndLenth2= 0,SndBuffLenth = 0;
static unsigned char InitPassed=0;
static unsigned short BlockSize=0;
static unsigned short AuxBuffer[6][44100/60]; //Biggest block size possible
static DWORD WritePointer=0;
static DWORD BuffOffset=0;
static char AuxBufferPointer=0;
static int CardCount=0;
static unsigned short CurrentRate=0;
static unsigned char AudioPause=0;
static SndCardList *Cards=NULL;
BOOL CALLBACK DSEnumCallback(LPGUID,LPCSTR,LPCSTR,LPVOID);





int SoundInit (HWND main_window_handle,_GUID * Guid,unsigned short Rate)
{
	Rate=(Rate & 3);
	CurrentRate=Rate;
	if (InitPassed)
	{
		InitPassed=0;
		lpdsbuffer1->Stop();

		if (lpdsbuffer1 !=NULL)
		{
			hr=lpdsbuffer1->Release();
			lpdsbuffer1=NULL;
		}

		if (lpds!=NULL)
		{
			hr=lpds->Release();
			lpds=NULL;
		}
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
		hr=DirectSoundCreate(Guid, &lpds, NULL);	// create a directsound object
		if (hr!=DS_OK)
			return(1);

		hr=lpds->SetCooperativeLevel(main_window_handle,DSSCL_NORMAL);//DSSCL_NORMAL);// set cooperation level normal DSSCL_EXCLUSIVE
		if (hr!=DS_OK)
			return(1);
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

		hr=lpds->CreateSoundBuffer(&dsbd,&lpdsbuffer1,NULL);
		if (hr!=DS_OK)
			return(1);

		// Clear out sound buffers
		hr= lpdsbuffer1->Lock(0,SndBuffLenth,&SndPointer1,&SndLenth1,&SndPointer2,&SndLenth2,DSBLOCK_ENTIREBUFFER );
		if (hr!=DS_OK)
			return(1);

		memset (SndPointer1,0,SndBuffLenth);
		hr=lpdsbuffer1->Unlock(SndPointer1,SndLenth1,SndPointer2,SndLenth2); 
		if (hr!=DS_OK)
			return(1);

		
		lpdsbuffer1->SetCurrentPosition(0);
		hr=lpdsbuffer1->Play(0,0,DSBPLAY_LOOPING );	// play the sound in looping mode
		if (hr!=DS_OK)
			return(1);
		InitPassed=1;
		AudioPause=0;
	}
	SetAudioRate (iRateList[Rate]);
	return(0);
}

void FlushAudioBuffer(unsigned int *Abuffer,unsigned short Lenth)
{
	unsigned short LeftAverage=0,RightAverage=0,Index=0;
	unsigned char Flag=0;
	unsigned char *Abuffer2=(unsigned char *)Abuffer;
	LeftAverage=Abuffer[0]>>16;
	RightAverage=Abuffer[0]& 0xFFFF;
	UpdateSoundBar(LeftAverage,RightAverage);
	if ((!InitPassed) | (AudioPause))
		return;
//	sprintf(Msg,"Lenth= %i Free Blocks= %i\n",Lenth,GetFreeBlockCount());
//	WriteLog(Msg,0);
	if (GetFreeBlockCount()<=0)	//this should only kick in when frame skipping or unthrottled
	{
		memcpy(AuxBuffer[AuxBufferPointer],Abuffer2,Lenth);	//Saving buffer to aux stack
		AuxBufferPointer++;		//and chase your own tail
		AuxBufferPointer%=5;	//At this point we are so far behind we may as well drop the buffer
		return;
	}

	hr=lpdsbuffer1->Lock(BuffOffset,Lenth,&SndPointer1,&SndLenth1,&SndPointer2,&SndLenth2,0);
	if (hr != DS_OK)
		return;
	
	memcpy(SndPointer1, Abuffer2, SndLenth1);	// copy first section of circular buffer
	if (SndPointer2 !=NULL)						// copy last section of circular buffer if wrapped
		memcpy(SndPointer2, Abuffer2+SndLenth1,SndLenth2);
	hr=lpdsbuffer1->Unlock(SndPointer1,SndLenth1,SndPointer2,SndLenth2);// unlock the buffer
	BuffOffset=(BuffOffset + Lenth)% SndBuffLenth;	//Where to write next
}


int GetFreeBlockCount(void) //return 0 on full buffer
 {
	unsigned long WriteCursor=0,PlayCursor=0;
	long RetVal=0,MaxSize=0;
	if ((!InitPassed) | (AudioPause))
		return(AUDIOBUFFERS);
	RetVal=lpdsbuffer1->GetCurrentPosition(&PlayCursor,&WriteCursor);
	if (BuffOffset <=PlayCursor)
		MaxSize= PlayCursor - BuffOffset;
	else
		MaxSize= SndBuffLenth - BuffOffset + PlayCursor;
	return(MaxSize/BlockSize); 

 }

 void PurgeAuxBuffer(void)
 {
	if ((!InitPassed) | (AudioPause))
		return;
	return;
	AuxBufferPointer--;			//Normally points to next free block Point to last used block
	if (AuxBufferPointer>=0)	//zero is a valid data block
	{
		while ((GetFreeBlockCount()<=0));


		hr=lpdsbuffer1->Lock(BuffOffset,BlockSize,&SndPointer1,&SndLenth1,&SndPointer2,&SndLenth2,0);//DSBLOCK_FROMWRITECURSOR);
		if (hr != DS_OK)
			return;
		memcpy(SndPointer1, AuxBuffer[AuxBufferPointer], SndLenth1);
		if (SndPointer2 !=NULL)
			memcpy(SndPointer2, (AuxBuffer[AuxBufferPointer]+(SndLenth1>>2)),SndLenth2);
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
	DirectSoundEnumerate(DSEnumCallback, NULL); 
	return(CardCount);
}

BOOL CALLBACK DSEnumCallback(LPGUID lpGuid,LPCSTR lpcstrDescription,LPCSTR lpcstrModule,LPVOID lpContext)          
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
		lpdsbuffer1->Stop();
		lpds->Release();
	}
	return(0);
}

int SoundInInit (HWND main_window_handle,_GUID * Guid)
{
	hr=DirectSoundCaptureCreate(Guid, &lpdsin, NULL);
	if (hr!=DS_OK)
		return(1);
	dsbdin.dwSize=sizeof(DSCBUFFERDESC); // Size of the structure
	dsbdin.dwFlags=0;
	dsbdin.dwReserved=0;
	dsbdin.lpwfxFormat=&pcmwf; //fix
	dsbdin.dwBufferBytes=SndBuffLenth;
	hr=lpdsin->CreateCaptureBuffer(&dsbdin, &lpdsbuffer2, NULL);
	if (hr!=DS_OK)
		return(1);
//	lpdsbuffer2->Initialize(lpdsin,&dsbdin);
	lpdsbuffer2->Start(hr);
	return(0);
}

unsigned short GetSoundStatus(void)
{
	return(CurrentRate);
}

void ResetAudio (void)
{
	SetAudioRate (iRateList[CurrentRate]);
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
	return(AudioPause);
}