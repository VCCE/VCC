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

/*****************************************************************************/
/*
    Audio module - currently disabled
 */
/*****************************************************************************/

#include "audio.h"

#include "xDebug.h"

#include "_audio.h"

#if 0
/*
 Audio
 */
confSetString(hConf,VCC_CONF_SECTION_AUDIO,"SndCard",pConfig->SoundCardName);
confSetInt(hConf,VCC_CONF_SECTION_AUDIO,"Rate",pConfig->AudioRate);


/*
 Audio
 */
if ( confGetInt(hConf,VCC_CONF_SECTION_AUDIO,"Rate",&iValue) == XERROR_NONE )
{
	pConfig->AudioRate		= iValue;
}

confGetString(hConf,VCC_CONF_SECTION_AUDIO,"SndCard",pConfig->SoundCardName,XTEXT_NUM_CHARS(pConfig->SoundCardName));



#if 0
for (iIndex=0; iIndex<pInstance->NumberOfSoundCards; iIndex++)
{
	if ( ! xStrCmp(pInstance->SoundCards[iIndex].CardName,pConfig->SoundCardName) )
	{
		pConfig->SndOutDev = iIndex;
	}
}
#endif

#endif

/*****************************************************************************/

//const char			RateList[4][7]	= {"Mute","11025","22050","44100"};
const unsigned short	iRateList[4]	= {0,11025,22050,44100};

/*****************************************************************************/
/**
	Initialize audio module
 */
audio_t * audCreate(void * hWindow, pguid_t pGuid, unsigned short Rate, unsigned short FrameRate)
{
	audio_t *	pAudio = NULL;
	
	// limit rate
	Rate = (Rate & 3);
	
	/* 
		create platform specific audio object 
	 
		'generic' variables are set there
	*/
	pAudio = _audInit(hWindow, pGuid, Rate, FrameRate);
	//assert(pAudio != NULL);
	if ( pAudio != NULL )
	{
		/*
			config default values
		 */
		pAudio->AudioRate			= 3;
		
		// TODO: handle this for each platform properly
		strcpy(pAudio->SoundCardName,"");

		/*
		 */
		if ( Rate )
		{
			pAudio->InitPassed = 1;
			pAudio->AudioPause = 0;
		}
	}
	
	return pAudio;
}

/*****************************************************************************/
/**
	Terminate audio module
 */
void audDeInit(audio_t * pAudio)
{
	if ( pAudio != NULL )
	{
		_audDeInit(pAudio);
		
		free(pAudio);
	}
}

/*****************************************************************************/
/**
	Add audio buffer to play
 */
void audFlushAudioBuffer(audio_t * pAudio, unsigned int * Abuffer, unsigned short Lenth)
{
	if ( pAudio != NULL )
	{
		if (     pAudio->InitPassed 
			 && !pAudio->AudioPause 
		   )
		{
			_audFlushAudioBuffer(pAudio, Abuffer, Lenth);
		}
	}
}

/*****************************************************************************/
/**
 */
int audGetSoundStatus(audio_t * pAudio)
{
	if ( pAudio != NULL )
	{
		return (pAudio->CurrentRate);
	}
	
	return 0;
}

/*****************************************************************************/
/**
 */
void audResetAudio(audio_t * pAudio)
{
	//coco3_t * pCoco3;
	
	if ( pAudio != NULL )
	{
		assert(0 && "what was this for???  make it into a callback?");
		//cc3SetAudioRate(pCoco3,iRateList[pAudio->CurrentRate]);

		_audResetAudio(pAudio);
	}
}

/*****************************************************************************/
/**
	Set pause staet of audio
 */
int audPauseAudio(audio_t * pAudio, unsigned char Pause)
{
	if ( pAudio != NULL )
	{
		pAudio->AudioPause = Pause;
	
		if ( pAudio->InitPassed )
		{
			_audPauseAudio(pAudio, Pause);
		}
	
		return (pAudio->AudioPause);
	}
	
	return FALSE;
}

/*****************************************************************************/

