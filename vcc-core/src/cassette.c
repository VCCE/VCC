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

/***********************************************************/
/**
	Cassette support
 
 TODO: Re-integrate
 
 TODO: minimum UI requirements (play/pause/record)
 TODO: write WAV properly so it can be read in by an actual Coco
 TODO: handle audio out from cassette
*/
/***********************************************************/
/**
	Functions you can call from the UI
 
	casMountTape
	casSetTapeMode

	casGetTapeFileName
	casGetTapeCounter
 */
/***********************************************************/

#include "Cassette.h"

#include "coco3.h"

#include "Vcc.h"

#include "xDebug.h"

#include <stdio.h>

/***********************************************************/
/***********************************************************/

#pragma mark -
#pragma mark - Local Macros


#define ASSERT_CASSETTE(pInstance)	assert(pInstance != NULL);	\
									assert(pInstance->device.id == EMU_DEVICE_ID); \
									assert(pInstance->device.idModule == VCC_CASSETTE_ID)

/***********************************************************/
/*
	Wave output for 0 or 1
 */

const unsigned char One[21]	= 
{
	0x80,0xA8,0xC8,0xE8,0xE8,0xF8,0xF8,0xE8,
	0xC8,0xA8,0x78,0x50,0x50,0x30,0x10,0x00,
	0x00,0x10,0x30,0x30,0x50
};
const unsigned char Zero[40] = 
{
	0x80,0x90,0xA8,0xB8,0xC8,0xD8,0xE8,0xE8,
	0xF0,0xF8,0xF8,0xF8,0xF0,0xE8,0xD8,0xC8,
	0xB8,0xA8,0x90,0x78,0x78,0x68,0x50,0x40,
	0x30,0x20,0x10,0x08,0x00,0x00,0x00,0x08,
	0x10,0x10,0x20,0x30,0x40,0x50,0x68,0x68
};

/***********************************************************/
/***********************************************************/

#pragma mark -
#pragma mark - Cassette utility functions

/***********************************************************/
/**
 */
void casCastoWav(cassette_t * pCassette, unsigned char *Buffer,unsigned int BytestoConvert,unsigned long *BytesConverted)
{	
	unsigned char Byte=0;
	char Mask=0;
	
	assert(pCassette != NULL);
	
	if ( pCassette->Quiet > 0 )
	{
		pCassette->Quiet--;
		memset(Buffer,0,BytestoConvert);
		return;
	}
	
	if ( (pCassette->TapeOffset > pCassette->TotalSize) | (pCassette->TotalSize == 0) )	//End of tape return nothing
	{
		memset(Buffer,0,BytestoConvert);
		pCassette->TapeMode = kTapeMode_Stop;	//Stop at end of tape
		return;
	}
	
	while ( (pCassette->TempIndex<BytestoConvert) & (pCassette->TapeOffset <= pCassette->TotalSize) )
	{
		Byte = pCassette->CasBuffer[(pCassette->TapeOffset++)%pCassette->TotalSize];
		for ( Mask=0; Mask<=7; Mask++ )
		{
			if ( (Byte & (1<<Mask)) == 0 )
			{
				memcpy(&pCassette->TempBuffer[pCassette->TempIndex],Zero,40);
				pCassette->TempIndex+=40;
			}
			else
			{
				memcpy(&pCassette->TempBuffer[pCassette->TempIndex],One,21);
				pCassette->TempIndex+=21;
			}
		}
	}
	
	if ( pCassette->TempIndex >= BytestoConvert )
	{
		memcpy(Buffer,pCassette->TempBuffer,BytestoConvert);									//Fill the return Buffer
		memcpy(pCassette->TempBuffer, &pCassette->TempBuffer[BytestoConvert],pCassette->TempIndex-BytestoConvert);	//Slide the overage to the front
		pCassette->TempIndex-=BytestoConvert;													//Point to the Next free byte in the tempbuffer
	}
	else	//We ran out of source bytes
	{
		memcpy(Buffer,pCassette->TempBuffer,pCassette->TempIndex);						//Partial Fill of return buffer;
		memset(&Buffer[pCassette->TempIndex],0,BytestoConvert-pCassette->TempIndex);		//and silence for the rest
		pCassette->TempIndex = 0;
	}
}

/***********************************************************/
/**
 */
void casWavtoCas(cassette_t * pCassette, unsigned char *WaveBuffer,unsigned int Lenth)
{
	unsigned char	Bit		= 0;
	unsigned char	Sample	= 0;
	unsigned int	Index	= 0;
	unsigned int	Width	= 0;
	
	assert(pCassette != NULL);

	for (Index=0; Index<Lenth; Index++)
	{
		Sample = WaveBuffer[Index];
		if ( (pCassette->LastSample <= 0x80) & (Sample>0x80))	//Low to High transition
		{
			Width = Index - pCassette->LastTrans;
			if ( (Width <10) | (Width >50) )	//Invalid Sample Skip it
			{
				pCassette->LastSample	=0;
				pCassette->LastTrans	=Index;
				pCassette->Mask			=0;
				pCassette->cByte=0;
			}
			else
			{
				Bit=1;
				if (Width >30)
					Bit=0;
				pCassette->cByte = pCassette->cByte | (Bit << pCassette->Mask);
				pCassette->Mask++;
				pCassette->Mask&=7;
				if (pCassette->Mask==0)
				{
					pCassette->CasBuffer[pCassette->TapeOffset++] = pCassette->cByte;
					pCassette->cByte = 0;
					if (pCassette->TapeOffset>= WRITEBUFFERSIZE)	//Don't blow past the end of the buffer
					{
						pCassette->TapeMode = kTapeMode_Stop;
					}
				}
				
			}
			pCassette->LastTrans=Index;
		} //Fi LastSample
		pCassette->LastSample=Sample;
	}	//Next Index
	pCassette->LastTrans-=Lenth;
	if (pCassette->TapeOffset>pCassette->TotalSize)
		pCassette->TotalSize=pCassette->TapeOffset;
}

/********************************************************************************/

#pragma mark -
#pragma mark --- forward declarations ---

void casCloseTapeFile(cassette_t * pCassette);

/********************************************************************************/

#pragma mark -
#pragma mark --- emulator device callbacks ---

/********************************************************************/

result_t casEmuDevDestroy(emudevice_t * pEmuDevice)
{
	cassette_t *	pCassette;
	
	pCassette = (cassette_t *)pEmuDevice;
	
	ASSERT_CASSETTE(pCassette);
	
	if ( pCassette != NULL )
	{
		casCloseTapeFile(pCassette);
		
		if ( pCassette->CasBuffer != NULL )
		{
			free(pCassette->CasBuffer);
		}
		
		emuDevRemoveChild(pCassette->device.pParent,&pCassette->device);
		
		free(pCassette);
	}
	
	return XERROR_NONE;
}

/********************************************************************************/

result_t casEmuDevConfSave(emudevice_t * pEmuDevice, config_t * config)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	cassette_t *	pCassette;
	
	pCassette = (cassette_t *)pEmuDevice;
	
	ASSERT_CASSETTE(pCassette);
	
	if ( pCassette != NULL )
	{
		confSetPath(config,CONF_SECTION_CASSETTE,CONF_SETTING_TAPEPATH,pCassette->pPathname, config->absolutePaths);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/********************************************************************************/

result_t casEmuDevConfLoad(emudevice_t * pEmuDevice, config_t * config)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	cassette_t *	pCassette;
	
	pCassette = (cassette_t *)pEmuDevice;
	
	ASSERT_CASSETTE(pCassette);
	
	if ( pCassette != NULL )
	{
		confGetPath(config,CONF_SECTION_CASSETTE,CONF_SETTING_TAPEPATH,&pCassette->pPathname,config->absolutePaths);
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/*********************************************************************************/

result_t casEmuDevGetStatus(emudevice_t * pEmuDevice, char * pszText, size_t szText)
{
	result_t		errResult	= XERROR_INVALID_PARAMETER;
	cassette_t *	pCassette;
	
	pCassette = (cassette_t *)pEmuDevice;
	
	ASSERT_CASSETTE(pCassette);
	
	if ( pCassette != NULL )
	{
		if ( pCassette->pPathname == NULL )
		{
			sprintf(pszText,"No tape");
		}
		else 
		{
			sprintf(pszText,"M:%d C:%d",pCassette->MotorState,0);
		}
		
		errResult = XERROR_NONE;
	}
	
	return errResult;
}

/***********************************************************/
/***********************************************************/

#pragma mark -
#pragma mark - Cassette object 

/***********************************************************/
/**
	Initialize cassette module
 */
cassette_t * casCreate()
{
	cassette_t *	pCassette = NULL;
	
	pCassette = calloc(1,sizeof(cassette_t));
	assert(pCassette != NULL);
	if ( pCassette != NULL )
	{
		pCassette->device.id		= EMU_DEVICE_ID;
		pCassette->device.idModule	= VCC_CASSETTE_ID;
		strcpy(pCassette->device.Name,EMU_DEVICE_NAME_CASSETTE);
		
		pCassette->device.pfnDestroy	= casEmuDevDestroy;
		pCassette->device.pfnSave		= casEmuDevConfSave;
		pCassette->device.pfnLoad		= casEmuDevConfLoad;
		pCassette->device.pfnGetStatus	= casEmuDevGetStatus;
		
		emuDevRegisterDevice(&pCassette->device);

		pCassette->MotorState		= 0;
		pCassette->TapeMode			= kTapeMode_Eject;
		pCassette->WriteProtect		= 0;
		pCassette->Quiet			= 30;
		//pCassette->TapeHandle		= NULL;
		pCassette->TapeOffset		= 0;
		pCassette->TotalSize		= 0;
		pCassette->CasBuffer		= NULL;
		pCassette->_FileType		= kTapeType_WAV;
		pCassette->BytesMoved		= 0;
		//pCassette->TapeFileName="";
		//pCassette->TempBuffer[8192];
	
	//Write Stuff
	//	pCassette->LastTrans		= 0;
		pCassette->Mask				= 0;
	//	pCassette->Byte				= 0;
		pCassette->LastSample		= 0;

		pCassette->TempIndex		= 0;
	}
	
	return pCassette;
}

/***********************************************************/
/**
 Write cassette file buffer
 */
void casSyncFileBuffer(cassette_t * pCassette)
{	
	assert(pCassette != NULL);
	
	//assert(0);
#if 0
	char			Buffer[64]		= "";
	unsigned long	BytesMoved		= 0;
	unsigned int	FileSize		= pCassette->TotalSize+40-8;
	unsigned short	WaveType		= 1;		//WAVE type format
	unsigned int	FormatSize		= 16;		//size of WAVE section chunck
	unsigned short	Channels		= 1;		//mono/stereo
	unsigned int	BitRate			= TAPEAUDIORATE;		//sample rate
	unsigned short	BitsperSample	= 8;	//Bits/sample
	unsigned int	BytesperSec		= BitRate*Channels*(BitsperSample/8);		//bytes/sec
	unsigned short	BlockAlign		= (BitsperSample * Channels)/8;		//Block alignment
	unsigned int	ChunkSize		= FileSize;
	
	xFileSeek(pCassette->TapeHandle,0,XFILE_SEEK_BEGIN);
	switch ( pCassette->_FileType )
	{
		case kTapeType_CAS:
			pCassette->CasBuffer[pCassette->TapeOffset]=pCassette->cByte;	//capture the last byte
			pCassette->LastTrans=0;	//reset all static inter-call variables
			pCassette->Mask=0;
			pCassette->cByte=0;	
			pCassette->LastSample=0;
			pCassette->TempIndex=0;
			xFileWrite(pCassette->TapeHandle,pCassette->TapeOffset,pCassette->CasBuffer,&BytesMoved);
			break;
			
		case kTapeType_WAV:
			// TODO: handle proper byte order
			sprintf(Buffer,"RIFF");
			xFileWrite(pCassette->TapeHandle,4,Buffer,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,4,&FileSize,&BytesMoved);
			sprintf(Buffer,"WAVE");
			xFileWrite(pCassette->TapeHandle,4,Buffer,&BytesMoved);
			sprintf(Buffer,"fmt ");
			xFileWrite(pCassette->TapeHandle,4,Buffer,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,4,&FormatSize,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,2,&WaveType,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,2,&Channels,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,4,&BitRate,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,4,&BytesperSec,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,2,&BlockAlign,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,2,&BitsperSample,&BytesMoved);
			sprintf(Buffer,"data");
			xFileWrite(pCassette->TapeHandle,4,Buffer,&BytesMoved);
			xFileWrite(pCassette->TapeHandle,4,&ChunkSize,&BytesMoved);
			break;
	}
#endif
	
	// TODO: handle flush of file
	//xFileFlush(pCassette->TapeHandle);
}

/***********************************************************/
/**
 */
void casCloseTapeFile(cassette_t * pCassette)
{
	assert(pCassette != NULL);
	
	//if ( pCassette->TapeHandle == NULL )
	//{
	//	return;
	//}
	
	casSyncFileBuffer(pCassette);
	
	//assert(0);
	//xFileClose(&pCassette->TapeHandle);
	
	pCassette->TotalSize = 0;
}

/***********************************************************/
/**
	Change state of cassette motor
 */
void casMotor(cassette_t * pCassette, unsigned char State)
{
	coco3_t *	pCoco3;
	
	assert(pCassette != NULL);
	pCoco3 = (coco3_t *)pCassette->device.pParent;
	
	pCassette->MotorState = State;
	switch ( pCassette->MotorState )
	{
		case 0:
			cc3SetSndOutMode(pCoco3,0);
			switch ( pCassette->TapeMode )
			{
				case kTapeMode_Stop:

				break;

				case kTapeMode_Play:
					pCassette->Quiet = 30;
					pCassette->TempIndex = 0;
				break;

				case kTapeMode_Record:
					casSyncFileBuffer(pCassette);
				break;

				case kTapeMode_Eject:

				break;
			}
		break;	//MOTOROFF

		case 1:
			switch ( pCassette->TapeMode )
			{
				case kTapeMode_Stop:
					cc3SetSndOutMode(pCoco3,0);
				break;

				case kTapeMode_Play:
					cc3SetSndOutMode(pCoco3,2);
				break;

				case kTapeMode_Record:
					cc3SetSndOutMode(pCoco3,1);
				break;

				case kTapeMode_Eject:
					cc3SetSndOutMode(pCoco3,0);
			}
		break;	//MOTORON	
	}
}

/***********************************************************/
/**
	Get current cassette tape counter
 */
size_t casGetTapeCounter(cassette_t * pCassette)
{
	assert(pCassette != NULL);
	
	return pCassette->TapeOffset;
}

/***********************************************************/
/**
	Set cassette tape counter
 */
#if 0
void casSetTapeCounter(cassette_t * pCassette, unsigned int Count)
{
	assert(pCassette != NULL);
	
	pCassette->TapeOffset = Count;
	if ( pCassette->TapeOffset > pCassette->TotalSize )
	{
		pCassette->TotalSize = pCassette->TapeOffset;
	}
	
#if 0
	// TODO: add UI hook
	UpdateTapeCounter(vccGetInstance(),pCassette->TapeOffset,pCassette->TapeMode);
#endif
}
#endif

/***********************************************************/
/**
	Set cassette tape mode
	
	@param pCassette Pointer to cassette module
	@param Mode New tape mode to change to
 
  Handles button pressed from Dialog
	@sa tapemode_e
 */
void casSetTapeMode(cassette_t * pCassette, unsigned char Mode)
{
	assert(pCassette != NULL);
	
	//assert(pCassette->TapeHandle != NULL);
	
	pCassette->TapeMode = Mode;
	switch ( pCassette->TapeMode )
	{
		case kTapeMode_Stop:

		break;

		case kTapeMode_Play:
			if ( pCassette->MotorState )
			{
				casMotor(pCassette,1);
			}
		break;

		case kTapeMode_Record:
		break;

		case kTapeMode_Eject:
			casCloseTapeFile(pCassette);
			//strcpy(pCassette->TapeFileName,"EMPTY");
		break;
	}
	
#if 0
	// TODO: add UI hook
	UpdateTapeCounter(vccGetInstance(),pCassette->TapeOffset,pCassette->TapeMode);
#endif
}

/***********************************************************/
/**
 */
void casFlushCassetteBuffer(cassette_t * pCassette, unsigned char * Buffer, uint32_t Lenth)
{
	assert(pCassette != NULL);
	
	assert(0);
#if 0
	unsigned long BytesMoved=0;

	assert(pCassette != NULL);
	
	if ( pCassette->TapeMode != kTapeMode_Record )
	{
		return;
	}
	
	switch ( pCassette->_FileType )
	{
		case kTapeType_WAV:
			xFileSeek(pCassette->TapeHandle, pCassette->TapeOffset+44, XFILE_SEEK_BEGIN);
			xFileWrite(pCassette->TapeHandle, Lenth, Buffer, &BytesMoved);
			if ( Lenth != BytesMoved )
			{
				return;
			}
			pCassette->TapeOffset += Lenth;
			if ( pCassette->TapeOffset > pCassette->TotalSize )
			{
				pCassette->TotalSize = pCassette->TapeOffset;
			}
			break;

		case kTapeType_CAS:
			casWavtoCas(pCassette, Buffer, Lenth);
		break;
	}
#endif
	
#if 0
	// TODO: add UI hook to update tape counter
	UpdateTapeCounter(pCassette,pCassette->TapeOffset,pCassette->TapeMode);
#endif
}

/***********************************************************/
/**
 */
void casLoadCassetteBuffer(cassette_t * pCassette, unsigned char * CassBuffer)
{
	assert(pCassette != NULL);
	
	assert(0);
#if 0
	unsigned long BytesMoved=0;
	
	if ( pCassette->TapeMode != kTapeMode_Play )
	{
		return;
	}
	
	switch ( pCassette->_FileType )
	{
		case kTapeType_WAV:
			xFileSeek(pCassette->TapeHandle, pCassette->TapeOffset+44, XFILE_SEEK_BEGIN);
			xFileRead(pCassette->TapeHandle,TAPEAUDIORATE/60,CassBuffer,&BytesMoved);
			pCassette->TapeOffset += BytesMoved;
			if ( pCassette->TapeOffset >pCassette-> TotalSize )
			{
				pCassette->TapeOffset = pCassette->TotalSize;
			}
			break;

		case kTapeType_CAS:
			casCastoWav(pCassette, CassBuffer, TAPEAUDIORATE/60, &BytesMoved);
			break;
	}
#endif
	
#if 0
	// TODO: add UI hook to update tape counter
	UpdateTapeCounter(pCassette, TapeOffset, TapeMode);
#endif
}

/***********************************************************/
/**
	@param pCassette
	@param FileName
 
	@return 1 on sucess 0 on fail
 */
int casMountTape(cassette_t * pCassette, const char * pPathname)
{
	assert(0);
#if 0
	char			Extension[4]	= "";
	unsigned char	Index			= 0;
	unsigned long	BytesMoved		= 0;
	
	assert(pCassette != NULL);
	
	if ( pCassette->TapeHandle != NULL )
	{
		pCassette->TapeMode = kTapeMode_Stop;
		casCloseTapeFile(pCassette);
	}

	pCassette->WriteProtect = 0;
	pCassette->_FileType	= 0;	// 0=wav 1=cas
	
	// TODO: handle creation of new audio file

	if ( xFileOpen(FileName,XFILE_MODE_READWRITE,&pCassette->TapeHandle) != XERROR_NONE )	
	{
		// Can't open read/write. try read only
		xFileOpen(FileName,XFILE_MODE_READ,&pCassette->TapeHandle);
		pCassette->WriteProtect = 1;
	}
	if ( pCassette->TapeHandle == NULL )
	{
		assert(0);
#if 0	
		// TODO: add UI hook for error
		MessageBox(0,"Can't Mount","Error",0);
#endif
		return(0);	//Give up
	}
	
	//xFileSeek(pCassette->TapeHandle,0,XFILE_SEEK_END);
	xFileGetSize(pCassette->TapeHandle,&pCassette->TotalSize);
	pCassette->TapeOffset = 0;
	strcpy(Extension,&FileName[strlen(FileName)-3]);
	for (Index=0; Index<strlen(Extension); Index++)
	{
		Extension[Index] = toupper(Extension[Index]);
	}
	
	if ( strcmp(Extension,"WAV") != 0 )
	{
		pCassette->_FileType = kTapeType_CAS;
		pCassette->LastTrans = 0;
		pCassette->Mask = 0;
		pCassette->cByte = 0;	
		pCassette->LastSample = 0;
		pCassette->TempIndex = 0;
		if ( pCassette->CasBuffer != NULL )
		{
			free(pCassette->CasBuffer);
		}
		
		pCassette->CasBuffer = (unsigned char *)calloc(1,WRITEBUFFERSIZE);
		//xFileSeek(pCassette->TapeHandle,0,XFILE_SEEK_BEGIN);
		// Read the whole file in for .CAS files
		xFileRead(pCassette->TapeHandle,pCassette->TotalSize,pCassette->CasBuffer,&BytesMoved);
		if ( BytesMoved != pCassette->TotalSize )
		{
			return(0);
		}
	}
#endif
	
	return (1);
}

/***********************************************************/
/**
 */
void casGetTapeName(cassette_t * pCassette, char * Name)
{
	assert(pCassette != NULL);
	
	assert(0);
	//strcpy(Name,pCassette->TapeFileName);
}

/***********************************************************/
