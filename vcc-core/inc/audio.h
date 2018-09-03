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

#ifndef _audio_h_
#define _audio_h_

#include "xTypes.h"

/*****************************************************************************/

typedef struct audio_t audio_t;

/*****************************************************************************/

#define TAPEAUDIORATE				44100
#define MAXAUDIORATE				44100

#define AUDIO_UPDATES_PER_SECOND	30
#define AUDIO_BUFFER_COUNT			AUDIO_UPDATES_PER_SECOND				//  
#define AUDIO_BUFFER_SIZE			MAXAUDIORATE*sizeof(int)/AUDIO_UPDATES_PER_SECOND	// 60 = 1s equiv of audio - max number of audio frames in each buffer - 16bit stereo (4 bytes ea) - 1/60 of a second.
																			//   if sample rate is lower, only a portion of the buffer is used 
/*****************************************************************************/

#define EMU_DEVICE_NAME_AUDIO	"Audio"
#define EMU_AUDIO_ID	XFOURCC('c','a','u','d')

#define CONF_SECTION_AUDIO		"Audio"

//#define CONF_SETTING_TAPEPATH	"TapePath"

/*****************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif
	
	audio_t *		audCreate(void * hWindow, void * pGuid, unsigned short Rate, unsigned short FrameRate);
	//void			audDeInit(audio_t *);

	void			audFlushAudioBuffer(audio_t *, unsigned int *, unsigned short);

	void			audResetAudio(audio_t *);
	int			    audPauseAudio(audio_t *, unsigned char Pause);
	int			    audGetSoundStatus(audio_t *);
	
#ifdef __cplusplus
}
#endif
		
#endif
