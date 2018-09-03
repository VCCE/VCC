/*****************************************************************************/
/*
	Windows version of audio layer
 
	OpenAL version
 
	Creates a single OpenAL source to play.  Data is queued on that source in
	small chunks.  The idea is to keep ahead of it so it always has data to play.
 
	Data is collected via _audFlushAudioBuffer.
*/
/*****************************************************************************/

#include "_audio.h"

#include "xDebug.h"

#include <assert.h>

#if (defined _WIN32)
#	include <al.h>
#	include <alc.h>
#else
#	include <OpenAL/al.h>
#	include <OpenAL/alc.h>
#endif

//#define AUDIO_DISABLED

#include "emuDevice.h"

/*****************************************************************************/

typedef struct audbuffer_t audbuffer_t;
struct audbuffer_t
{
	slink_t			link;
	
	ALuint			alBuffer;							// OpenAL buffer to play
	
	int				iDataIndex;							// current position/size of data in this buffer
	uint32_t		cData[AUDIO_BUFFER_SIZE];
} ;

typedef struct osxaudio_t osxaudio_t;
struct osxaudio_t
{
	audio_t			snd;
	
	ALCcontext *	Context;							// OpenAL context
	ALCdevice *		Device;								// OpenAL device to play through (default device)
	ALuint			alSource;							// OpenAL source - we only need one
	
	audbuffer_t		aryBuffers[AUDIO_BUFFER_COUNT];		// array of audio buffers to draw from
	slinklist_t		lstBuffersAvail;					// list of available buffers
	slinklist_t		lstBuffersReadyToPlay;				// list of buffers filled and ready to play (FILO)
	slinklist_t		lstBuffersQueued;					// list of buffers currently queued for playing with OpenAL
	audbuffer_t *	pBufferCurrent;						// current buffer being filled
};

#define MACOSX_AUDIO_ID	XFOURCC('o','s','x','a')

/*****************************************************************************/
/**
 display OpenAL error as a string
 */
void _alDisplayError(char * pString, ALint error)
{
	const char * pError = NULL;
	
	switch ( error )
	{
		case AL_NO_ERROR :
			//pError = "there is not currently an error";
			break;
			
		case AL_INVALID_NAME :
			pError = "a bad name (ID) was passed to an OpenAL function";
			break;
			
		case AL_INVALID_ENUM :
			pError = "an invalid enum value was passed to an OpenAL function";
			break;
			
		case AL_INVALID_VALUE :
			pError = "an invalid value was passed to an OpenAL function";
			break;
			
		case AL_INVALID_OPERATION :
			pError = "the requested operation is not valid";
			break;
			
		case AL_OUT_OF_MEMORY:
			pError = "the requested operation resulted in OpenAL running out of memory";
			break;
			
		default:
			pError = "unknown";
			break;
	}
	
	if ( pError != NULL )
	{
		XTRACE("%s : (%d) %s\n", pString, error, pError);
	}
}

/*****************************************************************************/
/**
	Create and initialize platform specific audio object
 */
// TODO: proper error checking
// TODO: remove Rate and target frame rate parameters
audio_t * _audInit(void * hWindow, void * pGuid, unsigned short Rate, int iTargetFrameRate)
{
#if !(defined AUDIO_DISABLED)
	osxaudio_t *	pOSXAudio;
	int				x;
	
	pOSXAudio = malloc(sizeof(osxaudio_t));
	assert(pOSXAudio != NULL);
	if ( pOSXAudio != NULL )
	{
		memset(pOSXAudio,0,sizeof(osxaudio_t));
		
		pOSXAudio->snd.device.id		= EMU_DEVICE_ID;
		pOSXAudio->snd.device.idModule	= EMU_AUDIO_ID;
		pOSXAudio->snd.device.idDevice	= MACOSX_AUDIO_ID;
		strcpy(pOSXAudio->snd.device.Name,"OSX Audio");
		
		emuDevRegisterDevice(&pOSXAudio->snd.device);

		assert(0);
		
		// move up to platform agnostic version
		pOSXAudio->snd.CurrentRate	= Rate;
		pOSXAudio->snd.BitRate		= iRateList[Rate];
		
		// initialize linked list of buffers
		slinklistClear(&pOSXAudio->lstBuffersAvail);
		for (x=0; x<AUDIO_BUFFER_COUNT; x++)
		{
			slinklistAddToTail(&pOSXAudio->lstBuffersAvail,&pOSXAudio->aryBuffers[x].link);
		}

		// get next available buffer
		pOSXAudio->pBufferCurrent = (audbuffer_t *)slinklistPopFront(&pOSXAudio->lstBuffersAvail);

		slinklistClear(&pOSXAudio->lstBuffersReadyToPlay);

		//if ( Rate )
		//{
			/*
				1. init OpenAL
				2. create one sound source
				3. create buffers
			 */
			pOSXAudio->Device = alcOpenDevice(NULL); //(ALubyte*)"DirectSound3D");
			if ( pOSXAudio->Device != NULL )
			{
				// Create context(s)
				pOSXAudio->Context = alcCreateContext(pOSXAudio->Device, NULL);
				assert(pOSXAudio->Context != NULL);
				
				//Set active context
				alcMakeContextCurrent(pOSXAudio->Context);
				assert(alGetError() == AL_NO_ERROR);
				
				alGenSources(1,&pOSXAudio->alSource);
				assert(alGetError() == AL_NO_ERROR);

				// pre-generate OpenAL buffers
				for (x=0; x<AUDIO_BUFFER_COUNT; x++)
				{
					alGenBuffers(1, &pOSXAudio->aryBuffers[x].alBuffer);
				}

				pOSXAudio->snd.InitPassed = 1;
			}
			else 
			{
				/* destroy audio? */
				assert(0);
			}
		//}
		
		return &pOSXAudio->snd;
	}
#endif
	
	return NULL;
}

/*****************************************************************************/
/**
	De-initialize audio
 */
void _audDeInit(audio_t * pAudio)
{
	osxaudio_t *	pOSXAudio = (osxaudio_t *)pAudio;
	int				x;
	
	if ( pOSXAudio != NULL )
	{
		assert(pOSXAudio->snd.device.idDevice == MACOSX_AUDIO_ID);
		
		//if ( pOSXAudio->snd.InitPassed )
		//{
			// To stop the sound
			alSourceStop(pOSXAudio->alSource);
			
			// delete our source
			alDeleteSources(1,&pOSXAudio->alSource);
			
			// delete our buffers
			for (x=0; x<AUDIO_BUFFER_COUNT; x++)
			{
				alDeleteBuffers(1, &pOSXAudio->aryBuffers[x].alBuffer);
			}	
		
			// Get active context
			pOSXAudio->Context = alcGetCurrentContext();
			
			// Get device for active context
			pOSXAudio->Device = alcGetContextsDevice(pOSXAudio->Context);
			
			// Disable context
			alcMakeContextCurrent(NULL);
			
			// Release context(s)
			alcDestroyContext(pOSXAudio->Context);
			pOSXAudio->Context = NULL;
			
			// Close device
			alcCloseDevice(pOSXAudio->Device);
			pOSXAudio->Device = NULL;
		//}
	}
}

/*****************************************************************************/
/**
	called to feed more data to the single playing sound
 
	ultimately called from the emulator at the end of each frame render 
	~60 times per second
 */
void _audFlushAudioBuffer(audio_t * pAudio, unsigned int * piBuffer, int iLength)
{
	osxaudio_t *	pOSXAudio	= (osxaudio_t *)pAudio;
	ALint			aliValue;
	ALint			error;
	int				iToCopy;
	unsigned int *	piWalk;
	int				iBufferSize;
	ALuint			alDequeuedBuffer;
	audbuffer_t *	pBuffer;
	audbuffer_t *	pBufferWalk;
	
	if ( pOSXAudio != NULL )
	{
		assert(pOSXAudio->snd.device.idDevice == MACOSX_AUDIO_ID);
	
		/*
			first copy the provided data to our current buffer used for collection
		*/
		/* the amount of data we use in each buffer is dependant on the current sample rate */
		iBufferSize	= iRateList[pAudio->CurrentRate]/AUDIO_UPDATES_PER_SECOND;
		assert(iBufferSize > 0 && iBufferSize <= AUDIO_BUFFER_SIZE);
		/* number of audio frames to copy */
		iToCopy = iLength;
		piWalk = piBuffer;
		while ( iToCopy > 0 )
		{
			/* copy 1 frame (16bit samples, stereo = 4 bytes) to temp buffer */
			pOSXAudio->pBufferCurrent->cData[pOSXAudio->pBufferCurrent->iDataIndex++] = *piWalk++;
			
			/* check if the current buffer is full */
			if ( pOSXAudio->pBufferCurrent->iDataIndex == iBufferSize )
			{
				// move full buffer to ready to play list
				slinklistAddToTail(&pOSXAudio->lstBuffersReadyToPlay,&pOSXAudio->pBufferCurrent->link);
				
				// get next current buffer
				pOSXAudio->pBufferCurrent = (audbuffer_t *)slinklistPopFront(&pOSXAudio->lstBuffersAvail);				
				// TODO: handle out of available buffers condition?
				// in theory this should never happen
				assert(pOSXAudio->pBufferCurrent != NULL);
				// reset write point
				pOSXAudio->pBufferCurrent->iDataIndex = 0;
				
			}
			
			// next audio sample frame
			iToCopy--;
		}

		/*
			De-queue all OpenAL buffers that are done playing
		 */
		do 
		{
			aliValue = 0;
			
			// first check if there are buffers on the source that are processed and can be re-used
			alGetSourcei(pOSXAudio->alSource, AL_BUFFERS_PROCESSED, &aliValue);
			error = alGetError();
			_alDisplayError("alGetSourcei",error);
			assert(error == AL_NO_ERROR);
			if ( error == AL_NO_ERROR
				 && aliValue > 0 
			   )
			{
				// grab next available buffer from source
				alSourceUnqueueBuffers(pOSXAudio->alSource, 1, &alDequeuedBuffer);
				_alDisplayError("alSourceUnqueueBuffers",error);
				assert((error = alGetError()) == AL_NO_ERROR);

				// find this buffer and remove from our queued list
				pBuffer = NULL;
				pBufferWalk = (audbuffer_t *)slinklistGetHead(&pOSXAudio->lstBuffersQueued);
				while ( pBufferWalk != NULL )
				{
					if ( pBufferWalk->alBuffer == alDequeuedBuffer )
					{
						pBuffer = pBufferWalk;
						break;
					}
					
					pBufferWalk = (audbuffer_t *)slinklistGetNext(&pBufferWalk->link);
				}
				assert(pBuffer != NULL);

				if ( pBuffer != NULL )
				{
					// remove from queued list
					slinklistRemove(&pOSXAudio->lstBuffersQueued, &pBuffer->link);
				
					// reset buffer
					pBuffer->iDataIndex = 0;
				
					// add to available list
					slinklistAddToTail(&pOSXAudio->lstBuffersAvail, &pBuffer->link);
				}
			}
		} while ( aliValue != 0 );
		
		/*
			purge ready queue if too far behind
			this happens when throttling is disabled
		 
			if too many buffers are in the ready to play list
				clear entire list and move to available queue
		 */
		// more than 1/3 of a second?
		if ( pOSXAudio->lstBuffersReadyToPlay.count > AUDIO_UPDATES_PER_SECOND/3 )
		{
			// purge ready to play queue
			do 
			{
				pBuffer = (audbuffer_t *)slinklistPopFront(&pOSXAudio->lstBuffersReadyToPlay);
				if ( pBuffer != NULL )
				{
					// reset
					pBuffer->iDataIndex = 0;
					
					// add to available list
					slinklistAddToTail(&pOSXAudio->lstBuffersAvail, &pBuffer->link);
				}
			} while (pBuffer != NULL);
		}

		/*
			Feed OpenAL source
		*/
		// no point doing anything if there is nothing ready to play
		if ( slinklistGetCount(&pOSXAudio->lstBuffersReadyToPlay) > 0 )
		{
			// check how many buffer are in the OpenAL queue
			alGetSourcei(pOSXAudio->alSource, AL_BUFFERS_QUEUED, &aliValue);
			error = alGetError();
			_alDisplayError("alGetSourcei",error);
			assert(error == AL_NO_ERROR);
			// if < 1/10 of a second queued
			if (    error == AL_NO_ERROR
				 && aliValue < AUDIO_UPDATES_PER_SECOND/10 
			   )
			{
				// pop next buffer ready to play
				pBuffer = (audbuffer_t *)slinklistPopFront(&pOSXAudio->lstBuffersReadyToPlay);
				if ( pBuffer != NULL )
				{
					// add to the queued list
					slinklistAddToTail(&pOSXAudio->lstBuffersQueued, &pBuffer->link);
					
					// fill OpenAL buffer 
					alBufferData(pBuffer->alBuffer, AL_FORMAT_STEREO16, (char *)pBuffer->cData, pBuffer->iDataIndex<<2, iRateList[pAudio->CurrentRate]);
					assert((error = alGetError()) == AL_NO_ERROR);
			
					// queue OpenAL buffer for playing
					alSourceQueueBuffers(pOSXAudio->alSource, 1, &pBuffer->alBuffer);
					assert((error = alGetError()) == AL_NO_ERROR);
			
					// make sure OpenAL source is playing
					alGetSourcei(pOSXAudio->alSource, AL_SOURCE_STATE, &aliValue);
					assert((error = alGetError()) == AL_NO_ERROR);
					if ( aliValue != AL_PLAYING )
					{
						alSourcePlay(pOSXAudio->alSource);
						assert(alGetError() == AL_NO_ERROR);
					}
				}
			}
		}
	}
}

/*****************************************************************************/
/**
 */
void _audResetAudio(audio_t * pAudio)
{
	osxaudio_t *	pOSXAudio = (osxaudio_t *)pAudio;
	audbuffer_t *	pBuffer;
	ALint			error;
	
	if ( pOSXAudio != NULL )
	{
		assert(pOSXAudio->snd.device.idDevice == MACOSX_AUDIO_ID);
		
		if ( pAudio->InitPassed )
		{
			// stop source
			alSourceStop(pOSXAudio->alSource);
			assert((error = alGetError()) == AL_NO_ERROR);

			// delete our source
			alDeleteSources(1,&pOSXAudio->alSource);
			assert((error = alGetError()) == AL_NO_ERROR);
			
			// recreate source
			alGenSources(1,&pOSXAudio->alSource);
			assert(alGetError() == AL_NO_ERROR);
			
			// reset and move all buffers from ready to play list to available list
			do 
			{
				pBuffer = (audbuffer_t *)slinklistPopFront(&pOSXAudio->lstBuffersReadyToPlay);
				if ( pBuffer != NULL )
				{
					// reset
					pBuffer->iDataIndex = 0;
					
					// add to available list
					slinklistAddToTail(&pOSXAudio->lstBuffersAvail, &pBuffer->link);
				}
			} while (pBuffer != NULL);
			
			// reset and move all buffers from queued list to available list
			do 
			{
				pBuffer = (audbuffer_t *)slinklistPopFront(&pOSXAudio->lstBuffersQueued);
				if ( pBuffer != NULL )
				{
					// reset
					pBuffer->iDataIndex = 0;
					
					// add to available list
					slinklistAddToTail(&pOSXAudio->lstBuffersAvail, &pBuffer->link);
				}
			} while (pBuffer != NULL);
			
			// reset current collection buffer too
			pOSXAudio->pBufferCurrent->iDataIndex = 0;
		}
	}
}

/*****************************************************************************/
/**
 */
void _audPauseAudio(audio_t * pAudio, int Pause)
{
	osxaudio_t * pOSXAudio = (osxaudio_t *)pAudio;
	
	if ( pOSXAudio != NULL )
	{
		assert(pOSXAudio->snd.device.idDevice == MACOSX_AUDIO_ID);
		
		if ( pAudio->InitPassed )
		{
			if ( Pause )
			{			
				// pause the sound
				alSourcePause(pOSXAudio->alSource);
			}
			else 
			{
				// play the sound
				alSourcePlay(pOSXAudio->alSource);
			}
		}
	}
}

/*****************************************************************************/
