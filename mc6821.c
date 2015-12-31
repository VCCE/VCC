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

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "defines.h"
#include "mc6821.h"
#include "hd6309.h"
#include "keyboard.h"
#include "tcc1014graphics.h"
#include "tcc1014registers.h"
#include "coco3.h"
#include "pakinterface.h"
#include "cassette.h"
#include "logger.h"
#include "resource.h"
#include <cstdint>

static unsigned char rega[4]={0,0,0,0};
static unsigned char regb[4]={0,0,0,0};
static unsigned char rega_dd[4]={0,0,0,0}; 
static unsigned char regb_dd[4]={0,0,0,0};
static unsigned char LeftChannel=0,RightChannel=0;
static unsigned char Asample=0,Ssample=0,Csample=0;
static unsigned char CartInserted=0,CartAutoStart=1;
static unsigned char AddLF=0;
static HANDLE hPrintFile=INVALID_HANDLE_VALUE;
void CaptureBit(unsigned char);
static HANDLE hout=NULL;
void WritePrintMon(char *);
LRESULT CALLBACK PrintMon(HWND, UINT , WPARAM , LPARAM );
static BOOL MonState=FALSE;

//static unsigned char CoutSample=0;
//extern STRConfig CurrentConfig;
// Shift Row Col
unsigned char pia0_read(unsigned char port)
{
	unsigned char dda,ddb;
	dda=(rega[1] & 4);
	ddb=(rega[3] & 4);

	switch (port)
	{
		case 1:
			return(rega[port]);
		break;

		case 3:
			return(rega[port]); 
		break;

		case 0:
			if (dda)
			{
				rega[1]=(rega[1] & 63);
				return(kb_scan(rega[2])); //Read
			}
			else
				return(rega_dd[port]);
		break;

		case 2: //Write 
			if (ddb)
			{
				rega[3]=(rega[3] & 63);
				return(rega[port] & rega_dd[port]);
			}
			else
				return(rega_dd[port]);
		break;
	}
	return(0);
}

unsigned char pia1_read(unsigned char port)
{
	static unsigned int Flag=0,Flag2=0;
	unsigned char dda,ddb;
	port-=0x20;
	dda=(regb[1] & 4);
	ddb=(regb[3] & 4);

	switch (port)
	{
		case 1:
		//	return(0);
		case 3:
			return(regb[port]);
		break;

		case 2:
			if (ddb)
			{
				regb[3]= (regb[3] & 63);

				return(regb[port] & regb_dd[port]);
			}
			else
				return(regb_dd[port]);
		break;

		case 0:
			if (dda)
			{
				regb[1]=(regb[1] & 63); //Cass In
				Flag=regb[port] ;//& regb_dd[port];
				return(Flag);
			}
			else
				return(regb_dd[port]);
		break;
	}
	return(0);
}

void pia0_write(unsigned char data,unsigned char port)
{
	unsigned char dda,ddb;
	dda=(rega[1] & 4);
	ddb=(rega[3] & 4);

	switch (port)
	{
	case 0:
		if (dda)
			rega[port]=data;
		else
			rega_dd[port]=data;
		return;
	case 2:
		if (ddb)
			rega[port]=data;
		else
			rega_dd[port]=data;
		return;
	break;

	case 1:
		rega[port]= (data & 0x3F);
		return;
	break;

	case 3:
		rega[port]= (data & 0x3F);
		return;
	break;	
	}
	return;
}

void pia1_write(unsigned char data,unsigned char port)
{
	unsigned char dda,ddb;
	static unsigned short LastSS=0;
	port-=0x20;

	dda=(regb[1] & 4);
	ddb=(regb[3] & 4);	
	switch (port)
	{
	case 0:
		if (dda)
		{
			regb[port]=data;
			CaptureBit((regb[0]&2)>>1);
			if (GetMuxState()==0)  
				if ((regb[3] & 8)!=0)//==0 for cassette writes
					Asample	= (regb[0] & 0xFC)>>1; //0 to 127
				else
					Csample = (regb[0] & 0xFC);
		}
		else
			regb_dd[port]=data;
		return;
	break;

	case 2: //FF22
		if (ddb)
		{
			regb[port]=(data & regb_dd[port]); 
			SetGimeVdgMode2( (regb[2] & 248) >>3);
			Ssample=(regb[port] & 2)<<6;
		}
		else
			regb_dd[port]=data;
		return;
	break;

	case 1:
		regb[port]= (data & 0x3F);
		Motor((data & 8)>>3);
		return;
	break;

	case 3:
		regb[port]= (data & 0x3F);
		return;
	break;
	}
	return;
}

unsigned char VDG_Mode(void)
{
	return( (regb[2] & 248) >>3);
}


void irq_hs(int phase)	//63.5 uS
{

	switch (phase)
	{
	case FALLING:	//HS went High to low
		if ( (rega[1] & 2) ) //IRQ on low to High transition
			return;
		rega[1]=(rega[1] | 128);
		if (rega[1] & 1)
			CPUAssertInterupt(IRQ,1);
	break;

	case RISING:	//HS went Low to High
		if ( !(rega[1] & 2) ) //IRQ  High to low transition
		return;
		rega[1]=(rega[1] | 128);
		if (rega[1] & 1)
			CPUAssertInterupt(IRQ,1);
	break;

	case ANY:	
		rega[1]=(rega[1] | 128);
		if (rega[1] & 1)
			CPUAssertInterupt(IRQ,1);
	break;
	} //END switch

	return;
}

void irq_fs(int phase)	//60HZ Vertical sync pulse 16.667 mS
{
	if ((CartInserted==1) & (CartAutoStart==1))
		AssertCart();
	switch (phase)
	{
	case 0:	//FS went High to low
		if ( (rega[3] & 2)==0 ) //IRQ on High to low transition
			rega[3]=(rega[3] | 128);
		if (rega[3] & 1)
			CPUAssertInterupt(IRQ,1);

		return;
	break;

	case 1:	//FS went Low to High

		if ( (rega[3] & 2) ) //IRQ  Low to High transition
		{
		rega[3]=(rega[3] | 128);
		if (rega[3] & 1)
			CPUAssertInterupt(IRQ,1);
		}
		return;
	break;
	} //END switch

	return;
}

void AssertCart(void)
{
	regb[3]=(regb[3] | 128);
	if (regb[3] & 1)
		CPUAssertInterupt(FIRQ,0);
	else
		CPUDeAssertInterupt(FIRQ); //Kludge but working
}

void PiaReset()
{	
	// Clear the PIA registers
	for (uint8_t index=0; index<4; index++)
	{
		rega[index]=0;
		regb[index]=0;
		rega_dd[index]=0;
		regb_dd[index]=0;
	}
}

unsigned char GetMuxState(void)
{
	return ( ((rega[1] & 8)>>3) + ((rega[3] & 8) >>2));
}

unsigned char DACState(void)
{
	return (regb[0]>>2);
}

void SetCart(unsigned char cart)
{
	CartInserted=cart;
	return;
}

unsigned int GetDACSample(void)
{
	static unsigned int RetVal=0;
	static unsigned short SampleLeft=0,SampleRight=0,PakSample=0;
	static unsigned short OutLeft=0,OutRight=0;
	static unsigned short LastLeft=0,LastRight=0;
	PakSample=PackAudioSample();
	SampleLeft=(PakSample>>8)+Asample+Ssample;
	SampleRight=(PakSample & 0xFF)+Asample+Ssample; //9 Bits each
	SampleLeft=SampleLeft<<6;	//Conver to 16 bit values
	SampleRight=SampleRight<<6;	//For Max volume
	if (SampleLeft==LastLeft)	//Simulate a slow high pass filter
	{
		if (OutLeft)
			OutLeft--;
	}
	else
	{
		OutLeft=SampleLeft;
		LastLeft=SampleLeft;
	}

	if (SampleRight==LastRight)
	{
		if (OutRight)
			OutRight--;
	}
	else
	{
		OutRight=SampleRight;
		LastRight=SampleRight;
	}

	RetVal=(OutLeft<<16)+(OutRight);
	return(RetVal);
}

unsigned char SetCartAutoStart(unsigned char Tmp)
{
	if (Tmp !=QUERY)
		CartAutoStart=Tmp;
	return(CartAutoStart);
}

unsigned char GetCasSample(void)
{
	return(Csample);
}

void SetCassetteSample(unsigned char Sample)
{
	regb[0]=regb[0] & 0xFE;
	if (Sample>0x7F)
		regb[0]=regb[0] | 1;
}

void CaptureBit(unsigned char Sample)
{
	unsigned long BytesMoved=0;
	static unsigned char BitMask=1,StartWait=1;
	static char Byte=0;
	if (hPrintFile==INVALID_HANDLE_VALUE)
		return;
	if (StartWait & Sample)	//Waiting for start bit
		return;
	if (StartWait)
	{
		StartWait=0;
		return;
	}
	if (Sample)
		Byte|=BitMask;
	BitMask=BitMask<<1;
	if (!BitMask)
	{
		BitMask=1;
		StartWait=1;
		WriteFile(hPrintFile,&Byte,1,&BytesMoved,NULL);
		if (MonState)
			WritePrintMon(&Byte);
		if ((Byte==0x0D) & AddLF)
		{
			Byte=0x0A;
			WriteFile(hPrintFile,&Byte,1,&BytesMoved,NULL);
		}
		Byte=0;
	}
	return;
}

int OpenPrintFile(char *FileName)
{
	hPrintFile=CreateFile( FileName,GENERIC_READ | GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	if (hPrintFile==INVALID_HANDLE_VALUE)
		return(0);
	return(1);
}

void ClosePrintFile(void)
{
	CloseHandle(hPrintFile);
	hPrintFile=INVALID_HANDLE_VALUE;
	FreeConsole();
	hout=NULL;
	return;
}

void SetSerialParams(unsigned char TextMode)
{
	AddLF=TextMode;
	return;
}

void SetMonState(BOOL State)
{
	if (MonState & !State)
	{
		FreeConsole();
		hout=NULL;
	}
	MonState=State;
	return;
}
void WritePrintMon(char *Data)
{
	unsigned long dummy;
	if (hout==NULL)
	{
		AllocConsole();
		hout=GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTitle("Printer Monitor"); 
	}
	WriteConsole(hout,Data,1,&dummy,0);
	if (Data[0]==0x0D)
	{
		Data[0]=0x0A;
		WriteConsole(hout,Data,1,&dummy,0);
	}
}

