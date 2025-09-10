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

#include <Windows.h>
#include <iostream>
#include "resource.h"
#include "defines.h"
#include "coco3.h"
#include "config.h"
#include "DialogOps.h"
#include "Cassette.h"
#include "stdio.h"
#include "logger.h"
#include <functional>

unsigned char MotorState=0,TapeMode=STOP,WriteProtect=0,Quiet=30;
static HANDLE TapeHandle=nullptr;
static unsigned long TapeOffset=0,TotalSize=0;
static char TapeFileName[MAX_PATH]="";
static char CassPath[MAX_PATH];
static unsigned char TempBuffer[8192];
static unsigned char *CasBuffer=nullptr;
static char TapeWritten = 0;
static char FileType=0;
unsigned long BytesMoved=0;
static unsigned int TempIndex=0;
static unsigned int TapeRate=CAS_TAPEAUDIORATE;
static unsigned int TapeSampleSize=0;
static unsigned int MotorOffDelay = 0;
static unsigned long PrvOffset=0,StalledCtr=0;

unsigned char One[21]={0xC8,0xE8,0xE8,0xF8,0xF8,0xE8,0xC8,0xA8,0x78,0x50,0x50,0x30,0x10,0x00,0x00,0x10,0x30,0x30,0x50,0x80,0xA8};
unsigned char Zero[40]={0xC8,0xD8,0xE8,0xE8,0xF0,0xF8,0xF8,0xF8,0xF0,0xE8,0xD8,0xC8,0xB8,0xA8,0x90,0x78,0x78,0x68,0x50,0x40,0x30,0x20,0x10,0x08,0x00,0x00,0x00,0x08,0x10,0x10,0x20,0x30,0x40,0x50,0x68,0x68,0x80,0x90,0xA8,0xB8};

namespace VCC
{
	struct WavHeader
	{
		char riff[4];		// RIFF
		uint32_t fileSize;
		char wave[4];		// WAVE
	};

	struct WavBlock
	{
		char name[4];		// "data"
		uint32_t size;
	};

	struct WavFmtBlock : WavBlock
	{
		uint16_t waveType;
		uint16_t channels;
		uint32_t bitRate;
		uint32_t bytesPerSec;
		uint16_t blockAlign;
		uint16_t bitsPerSample;
	};

	void SampleSignedConversion(int width, uint8_t* outBuffer, const uint8_t* inBuffer, size_t samples)
	{
		for (size_t i = 0; i < samples; ++i)
		{
			auto p = inBuffer + i * width + width - 1;
			int sample = *p;
			outBuffer[i] = (uint8_t)(sample + CAS_SILENCE);
		}
	}

	void SampleUnsignedConversion(int width, uint8_t* outBuffer, const uint8_t* inBuffer, size_t samples)
	{
		for (size_t i = 0; i < samples; ++i)
		{
			auto p = inBuffer + i * width + width - 1;
			uint8_t sample = *p;
			outBuffer[i] = sample;
		}
	}

	void SampleFloatConversion(int width, uint8_t* outBuffer, const uint8_t* inBuffer, size_t samples)
	{
		for (size_t i = 0; i < samples; ++i)
		{
			auto p = inBuffer + i * width;
			float sample = *reinterpret_cast<const float*>(p);
			outBuffer[i] = (uint8_t)(sample * 128) + 256 / 2;
		}
	}

	bool Init()
	{
		// reduce peak to peak to 75% for .cas as its more realistic what actual tape uses
		for (int i = 0; i < sizeof(One); ++i)
			One[i] = One[i] / 2 + One[i] / 4 + 64 / 2;
		for (int i = 0; i < sizeof(Zero); ++i)
			Zero[i] = Zero[i] / 2 + Zero[i] / 4 + 64 / 2;
		return true;
	}

	static bool gInit = Init();
}

static std::function<void(int, uint8_t*, const uint8_t*, size_t)> Conversion;

//Write Stuff
static int LastTrans=0;
static unsigned char Mask=0,Byte=0,LastSample=0;
//****************************

void WavtoCas(const unsigned char *,unsigned int);
int MountTape( const char *);
void CloseTapeFile();
void SyncFileBuffer();
void CastoWav(unsigned char *,unsigned int);

unsigned int GetTapeRate()
{
	return TapeRate;
}

unsigned char GetMotorState()
{
	if (MotorOffDelay > 0)
	{
		--MotorOffDelay;
		return 1;
	}
	return MotorState;
}

void Motor(unsigned char State)
{
	MotorState=State;
	switch (MotorState)
	{
		// MOTOROFF
		case 0:
			SetSndOutMode(0);
			switch (TapeMode)
			{
				case STOP:
				break;

				case PLAY:
					Quiet = 15;
					TempIndex = 0;
					MotorOffDelay = 10;
				break;

				case REC:
					SyncFileBuffer();
				break;

				case EJECT:
				break;
			}
		break;

		// MOTORON	
		case 1:
			switch (TapeMode)
			{
				case STOP:
					SetSndOutMode(0);
				break;

				case PLAY:
					SetSndOutMode(2);
				break;

				case REC:
					SetSndOutMode(1);
				break;

				case EJECT:
					SetSndOutMode(0);
			}
		break;
	}
	return;
}

unsigned int GetTapeCounter()
{
	return TapeOffset;
}

void SetTapeCounter(unsigned int Count, bool forced)
{
	TapeOffset=Count;
	if (TapeOffset>TotalSize)
		TotalSize=TapeOffset;
	UpdateTapeCounter(TapeOffset,TapeMode,forced);
	return;
}

void UpdateTapeStatus(char* status, int max)
{
	if (PrvOffset != TapeOffset) {
		StalledCtr = 0;
		PrvOffset = TapeOffset;
	} else {
		StalledCtr++;
	}
	if (TotalSize > 0 && StalledCtr < 1000)
		snprintf(status, max, " | Tape:%05lu (%lu%%)", TapeOffset, (TapeOffset + 50) * 100 / TotalSize);
}

void SetTapeMode(unsigned char Mode)	//Handles button pressed from Dialog
{
	TapeMode=Mode;
	switch (TapeMode)
	{
		case STOP:

		break;

		case PLAY:
			if (TapeHandle == nullptr)
			{
				if (!LoadTape())
				{
					TapeMode = STOP;
				}
				else
				{
					TapeMode = Mode;
				}
			}

			if (MotorState)
			{
				Motor(1);
			}
		break;

		case REC:
			if (TapeHandle == nullptr)
			{
				if (!LoadTape())
				{
					TapeMode = STOP;
				}
				else
				{
					TapeMode = Mode;
				}
			}
		break;

		case EJECT:
			CloseTapeFile();
			strcpy(TapeFileName,"EMPTY");
		break;
	}
	UpdateTapeCounter(TapeOffset,TapeMode);
	return;
}

void FlushCassetteBuffer(const unsigned char *Buffer,unsigned int *Len)
{
	if (TapeMode!=REC) 
		return;

	unsigned int Length = *Len;
	*Len = 0;

	TapeWritten = true;
	switch(FileType)
	{
	case WAV:
		SetFilePointer(TapeHandle,TapeOffset+44,nullptr,FILE_BEGIN);
		WriteFile(TapeHandle,Buffer,Length,&BytesMoved,nullptr);
		if (Length!=BytesMoved)
			return;
		TapeOffset+=Length;
		if (TapeOffset>TotalSize)
			TotalSize=TapeOffset;
	break;

	case CAS:
		WavtoCas(Buffer, Length);
		break;
	}
	UpdateTapeCounter(TapeOffset,TapeMode);
	return;
}

void LoadCassetteBuffer(unsigned char *CassBuffer, unsigned int* CassBufferSize)
{
	unsigned long BytesMoved=0;
	if (TapeMode != PLAY)
	{
		*CassBufferSize = TapeRate / 60;
		memset(&CassBuffer[0], CAS_SILENCE, *CassBufferSize);
		return;
	}

	unsigned int offset = TapeOffset;
	switch (FileType)
	{
		case WAV:
		{
			if (TapeHandle)
			{
				SetFilePointer(TapeHandle, TapeOffset + 44, nullptr, FILE_BEGIN);
				ReadFile(TapeHandle, TempBuffer, TapeSampleSize * TapeRate / 60, &BytesMoved, nullptr);
				if (BytesMoved > 0)
				{
					unsigned int samples = BytesMoved / TapeSampleSize;
					Conversion(TapeSampleSize, CassBuffer, TempBuffer, samples);
					*CassBufferSize = samples;
					TapeOffset += BytesMoved;
				}
				if (TapeOffset > TotalSize)
					TapeOffset = TotalSize;
			}
			break;
		}

		case CAS:
		{
			CastoWav(CassBuffer, CAS_TAPEREADAHEAD);
			*CassBufferSize = CAS_TAPEREADAHEAD;
			break;
		}
	}
	if (TapeOffset == TotalSize) offset = TapeOffset;
	UpdateTapeCounter(offset,TapeMode);
	return;
}

int MountTape( const char *FileName)	//Return 1 on sucess 0 on fail
{
	char Extension[4]="";
	unsigned char Index=0;
	if (TapeHandle!=nullptr)
	{
		TapeMode=STOP;
		CloseTapeFile();
	}

	WriteProtect=0;
	FileType=0;	//0=wav 1=cas
	TapeHandle = CreateFile(FileName,GENERIC_READ | GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr); //CREATE_NEW OPEN_ALWAYS
	if (TapeHandle == INVALID_HANDLE_VALUE)	//Can't open read/write. try read only
	{
		TapeHandle = CreateFile(FileName,GENERIC_READ,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
		WriteProtect=1;
	}
	if (TapeHandle==INVALID_HANDLE_VALUE)
	{
		MessageBox(nullptr,"Can't Mount","Error",0);
		return 0;	//Give up
	}
	TapeWritten = false;
	TotalSize=SetFilePointer(TapeHandle,0,nullptr,FILE_END);
	TapeOffset=0;
	TapeRate = CAS_TAPEAUDIORATE;
	TapeSampleSize = 1;
	SetFilePointer(TapeHandle, 0, nullptr, FILE_BEGIN);
	strcpy(Extension,&FileName[strlen(FileName)-3]);
	for (Index=0;Index<strlen(Extension);Index++)
		Extension[Index]=toupper(Extension[Index]);

	if (CasBuffer != nullptr)
	{
		free(CasBuffer);
		CasBuffer = nullptr;
	}

	if (strcmp(Extension, "WAV") == 0)
	{
		using namespace VCC;
		using namespace std::placeholders;

		WavHeader header;
		WavFmtBlock formatBlock;
		ReadFile(TapeHandle, &header, sizeof(header), &BytesMoved, nullptr);
		ReadFile(TapeHandle, &formatBlock, sizeof(formatBlock), &BytesMoved, nullptr);
		Conversion = nullptr;
		if (formatBlock.waveType == 1)
		{
			TapeSampleSize = formatBlock.channels * formatBlock.bitsPerSample / 8;
			if (formatBlock.bitsPerSample == 8)
				Conversion = SampleUnsignedConversion;
			else
				Conversion = SampleSignedConversion;
		}
		else if (formatBlock.waveType == 3)
		{
			TapeSampleSize = formatBlock.channels * formatBlock.bitsPerSample / 8;
			if (formatBlock.bitsPerSample == 32)
				Conversion = SampleFloatConversion;
		}
		if (!Conversion) return 0;
		CasBuffer = (unsigned char*)malloc(CAS_WRITEBUFFERSIZE);
		FileType = WAV;
		TapeRate = formatBlock.bitRate;
	}
	else if (strcmp(Extension, "CAS") == 0)
	{
		FileType=CAS;
		LastTrans=0;
		Mask=0;
		Byte=0;	
		LastSample=0;
		TempIndex=0;

		if (TotalSize > CAS_WRITEBUFFERSIZE)
			TotalSize = CAS_WRITEBUFFERSIZE;

		CasBuffer=(unsigned char *)malloc(CAS_WRITEBUFFERSIZE);
		ReadFile(TapeHandle,CasBuffer,TotalSize,&BytesMoved,nullptr);	//Read the whole file in for .CAS files
		if (BytesMoved!=TotalSize)
			return 0;
	}

	SetSndOutMode(2);

	return 1;
}

void CloseTapeFile()
{
	if (TapeHandle==nullptr)
		return;
	SyncFileBuffer();
	CloseHandle(TapeHandle);
	TapeHandle=nullptr;
	TapeRate = CAS_TAPEAUDIORATE;
	TotalSize=0;
}

unsigned int LoadTape()
{
	FileDialog dlg;
	char IniFilePath[MAX_PATH];
	GetIniFilePath(IniFilePath);
	GetPrivateProfileString("DefaultPaths","CassPath","",CassPath,MAX_PATH,IniFilePath);
	dlg.setInitialDir(CassPath);
	dlg.setFilter("Cassette Files (.cas,.wav)\0*.cas;*.wav\0\0");
	dlg.setTitle("Insert Tape Image");
	dlg.setFlags(OFN_NOTESTFILECREATE);
	if (dlg.show()) {
		dlg.getpath(TapeFileName,MAX_PATH);
		if (MountTape(TapeFileName)==0)	{
			MessageBox(nullptr,"Can't open file","Error",0);
			return 0;
		}
	}
	dlg.getdir(CassPath);
	if (strcmp(CassPath, "") != 0)
		WritePrivateProfileString("DefaultPaths","CassPath",CassPath,IniFilePath);
	// turn off fast load for wav files
	if (FileType == WAV) TapeFastLoad = false;
	TapeWritten = false;
	return 1;
}

void GetTapeName(char *Name)
{
	strcpy(Name,TapeFileName);
	return;
}

void SyncFileBuffer ()
{
	if (!TapeWritten) return;
	char Buffer[64]="";
	unsigned long BytesMoved=0;
	unsigned int FileSize=TotalSize+40-8;
	unsigned short WaveType=1;		//WAVE type format
	unsigned int FormatSize=16;		//size of WAVE section chunck
	unsigned short Channels=1;		//mono/stereo
	unsigned int BitRate=TapeRate;		//sample rate
	unsigned short BitsperSample=8;	//Bits/sample
	unsigned int BytesperSec=BitRate*Channels*(BitsperSample/8);		//bytes/sec
	unsigned short BlockAlign=(BitsperSample * Channels)/8;		//Block alignment
	unsigned int ChunkSize=FileSize;

	SetFilePointer(TapeHandle,0,nullptr,FILE_BEGIN);
	switch (FileType)
	{
	case CAS:
		CasBuffer[TapeOffset]=Byte;	//capture the last byte
		LastTrans=0;	//reset all static inter-call variables
		Mask=0;
		Byte=0;	
		LastSample=0;
		TempIndex=0;
		WriteFile(TapeHandle,CasBuffer,TapeOffset,&BytesMoved,nullptr);
	break;

	case WAV:
		sprintf(Buffer,"RIFF");
		WriteFile(TapeHandle,Buffer,4,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&FileSize,4,&BytesMoved,nullptr);
		sprintf(Buffer,"WAVE");
		WriteFile(TapeHandle,Buffer,4,&BytesMoved,nullptr);
		sprintf(Buffer,"fmt ");
		WriteFile(TapeHandle,Buffer,4,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&FormatSize,4,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&WaveType,2,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&Channels,2,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&BitRate,4,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&BytesperSec,4,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&BlockAlign,2,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&BitsperSample,2,&BytesMoved,nullptr);
		sprintf(Buffer,"data");
		WriteFile(TapeHandle,Buffer,4,&BytesMoved,nullptr);
		WriteFile(TapeHandle,&ChunkSize,4,&BytesMoved,nullptr);
	break;
	}
	FlushFileBuffers(TapeHandle);
	return;
}


void CastoWav(unsigned char *Buffer,unsigned int BytestoConvert)
{	
	unsigned char Byte=0;
	char Mask=0;

	assert((BytestoConvert & 1) == 0);

	// copy any left over bytes and fill remaining space with silence
	auto fillSilence = [&]()
	{
		int remaining = TempIndex - BytestoConvert;
		if (TempIndex)
			memcpy(Buffer, TempBuffer, TempIndex);						//Partial Fill of return buffer;
		memset(&Buffer[TempIndex], CAS_SILENCE, -remaining);					//and silence for the rest
		TempIndex = 0;
	};

	if (Quiet>0)
	{
		Quiet--;
		fillSilence();
		return;
	}

	if (TapeOffset >= TotalSize || TotalSize == 0)	//End of tape return nothing
	{
		TapeMode=STOP;	//Stop at end of tape
		fillSilence();
		return;
	}

	while (TempIndex < BytestoConvert && TapeOffset < TotalSize)
	{
		Byte=CasBuffer[(TapeOffset++)%TotalSize];
		if (TapeFastLoad)
		{
			for (Mask = 0; Mask <= 7; ++Mask, Byte >>= 1)
			{
				// color basic expects high/low transitions so this
				// is the smallest waveform that we can have without 
				// hacking the rom. CA
				// high/low waveform (b1) + tape bit (b0)
				TempBuffer[TempIndex++] = 2 + (Byte & 1);
				TempBuffer[TempIndex++] = 0 + (Byte & 1);
			}
		}
		else
		{
			for (Mask = 0;Mask <= 7;Mask++)
			{
				if ((Byte & (1 << Mask)) == 0)
				{
					memcpy(&TempBuffer[TempIndex],Zero,40);
					TempIndex+=40;
				}
				else
				{
					memcpy(&TempBuffer[TempIndex],One,21);
					TempIndex+=21;
				}
			}
		}
	}
	
	int remaining = TempIndex - BytestoConvert;
	if (remaining >= 0)
	{
		memcpy(Buffer,TempBuffer,BytestoConvert);						//Fill the return Buffer
		if (remaining > 0)
			memcpy(TempBuffer, &TempBuffer[BytestoConvert], remaining);	//Slide the overage to the front
		TempIndex-=BytestoConvert;										//Point to the Next free byte in the tempbuffer
		return;
	}

	fillSilence();
}


void WavtoCas(const unsigned char *WaveBuffer,unsigned int Length)
{
	unsigned char Bit=0,Sample=0;
	unsigned int Index=0,Width=0;

	for (Index=0;Index<Length;Index++)
	{
		Sample=WaveBuffer[Index];
		if ( (LastSample <= 0x80) & (Sample>0x80))	//Low to High transition
		{
			Width=Index-LastTrans;
			if ((Width <10) | (Width >50))	//Invalid Sample Skip it
			{
				LastSample=0;
				LastTrans=Index;
				Mask=0;
				Byte=0;
			}
			else
			{
				Bit=1;
				if (Width >30)
					Bit=0;
				Byte=Byte | (Bit<<Mask);
				Mask++;
				Mask&=7;
				if (Mask==0)
				{
					CasBuffer[TapeOffset++]=Byte;
					Byte=0;
					if (TapeOffset>= CAS_WRITEBUFFERSIZE)	//Don't blow past the end of the buffer
						TapeMode=STOP;
				}

			}
			LastTrans=Index;
		} //Fi LastSample
		LastSample=Sample;
	}	//Next Index
	LastTrans-=Length;
	if (TapeOffset>TotalSize)
		TotalSize=TapeOffset;
	return;
}
