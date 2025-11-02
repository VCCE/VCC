////////////////////////////////////////////////////////////////////////////////
//	Copyright 2015 by Joseph Forgione
//	This file is part of VCC (Virtual Color Computer).
//	
//	VCC (Virtual Color Computer) is free software: you can redistribute itand/or
//	modify it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or (at your
//	option) any later version.
//	
//	VCC (Virtual Color Computer) is distributed in the hope that it will be
//	useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
//	Public License for more details.
//	
//	You should have received a copy of the GNU General Public License along with
//	VCC (Virtual Color Computer). If not, see <http://www.gnu.org/licenses/>.
////////////////////////////////////////////////////////////////////////////////
#include "utils.h"
#include "wd1793.h"
#include <Windows.h>


long CreateDiskHeader(const char *FileName,unsigned char Type,unsigned char Tracks,unsigned char DblSided)
{
	HANDLE hr=nullptr;
	unsigned char Dummy=0;
	unsigned char HeaderBuffer[16]="";
	unsigned char TrackTable[3]={35,40,80};
	unsigned short TrackSize=0x1900;
	unsigned char IgnoreDensity=0,SingleDensity=0,HeaderSize=0;
	unsigned long BytesWritten=0,FileSize=0;
	hr=CreateFile( FileName,GENERIC_READ | GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
	if (hr==INVALID_HANDLE_VALUE)
		return 1; //Failed to create File

	switch (Type)
	{
	case DMK:
		HeaderBuffer[0]=0;
		HeaderBuffer[1]=TrackTable[Tracks];
		HeaderBuffer[2]=(TrackSize & 0xFF);
		HeaderBuffer[3]=(TrackSize >>8);
		HeaderBuffer[4]=(IgnoreDensity<<7) | (SingleDensity<<6) | ((!DblSided)<<4);
		HeaderBuffer[0xC]=0;
		HeaderBuffer[0xD]=0;
		HeaderBuffer[0xE]=0;
		HeaderBuffer[0xF]=0;
		HeaderSize=0x10;
		FileSize= HeaderSize + (HeaderBuffer[1] * TrackSize * (DblSided+1) );
		break;

	case JVC:	// has variable header size
		HeaderSize=0;
		HeaderBuffer[0]=18;			//18 Sectors
		HeaderBuffer[1]=DblSided+1;	// Double or single Sided;
		FileSize = (HeaderBuffer[0] * 0x100 * TrackTable[Tracks] * (DblSided+1));
		if (DblSided)
		{
			FileSize+=2;
			HeaderSize=2;
		}
		break;

	case VDK:
		HeaderBuffer[9]=DblSided+1;
		HeaderSize=12;
		FileSize = ( 18 * 0x100 * TrackTable[Tracks] * (DblSided+1));
		FileSize+=HeaderSize;
		break;

	case 3:
		HeaderBuffer[0]=0;
		HeaderBuffer[1]=TrackTable[Tracks];
		HeaderBuffer[2]=(TrackSize & 0xFF);
		HeaderBuffer[3]=(TrackSize >>8);
		HeaderBuffer[4]=((!DblSided)<<4);
		HeaderBuffer[0xC]=0x12;
		HeaderBuffer[0xD]=0x34;
		HeaderBuffer[0xE]=0x56;
		HeaderBuffer[0xF]=0x78;
		HeaderSize=0x10;
		FileSize=1;
		break;

	}
	SetFilePointer(hr,0,nullptr,FILE_BEGIN);
	WriteFile(hr,HeaderBuffer,HeaderSize,&BytesWritten,nullptr);
	SetFilePointer(hr,FileSize-1,nullptr,FILE_BEGIN);
	WriteFile(hr,&Dummy,1,&BytesWritten,nullptr);
	CloseHandle(hr);
	return 0;
}
