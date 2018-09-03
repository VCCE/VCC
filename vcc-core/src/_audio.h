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

#ifndef __audio_h_
#define __audio_h_

/*****************************************************************************/

#include "audio.h"
#include "emuDevice.h"

/*****************************************************************************/

typedef void * pguid_t;

typedef struct CardList 
{
	char		CardName[64];
	pguid_t		Guid;
} SndCardList;

struct audio_t
{
	emudevice_t		device;

	/*
		config / persistence
	 */
	//	unsigned char	AudioMute;
	int 			AudioRate;
	int 			SndOutDev;
	char			SoundCardName[64];

	/*
		run time
	 */
	int				CardCount;
	SndCardList *	Cards;
	
	int 			InitPassed;

	int 			BitRate;
	int 			CurrentRate;
	int 			AudioPause;
};

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

	extern const unsigned short	iRateList[4];
	
	audio_t *		_audInit(void * hWindow, void * pGuid, unsigned short Rate, int iTargetFrameRate);
	void			_audDeInit(audio_t * pAudio);
	void			_audFlushAudioBuffer(audio_t * pAudio, unsigned int * piBuffer, int iLength);
	void			_audResetAudio(audio_t * pAudio);
	void			_audPauseAudio(audio_t * pAudio, int Pause);
	//int				_audGetFreeBlockCount(audio_t *);
	int				_audGetAuxBlockCount(audio_t * pAudio);

#ifdef __cplusplus
}
#endif
	
/*****************************************************************************/

#endif
