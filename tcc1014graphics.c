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
#include "defines.h"
#include "tcc1014graphics.h"
#include "ddraw.h"
#include "coco3.h"
#include "cc2font.h"
#include "cc3font.h"
#include "config.h"
#include "DirectDrawInterface.h"
#include "logger.h"
#include "math.h"
#include <stdio.h>
#include <commctrl.h>	// Windows common controls

void SetupDisplay(void); //This routine gets called every time a software video register get updated.
void MakeRGBPalette (void);
void MakeCMPpalette(void);
bool  DDFailedCheck(HRESULT hr, char *szMessage);
char *DDErrorString(HRESULT hr);
//extern STRConfig CurrentConfig;
static unsigned char ColorValues[4]={0,85,170,255};
static unsigned char ColorTable16Bit[4]={0,10,21,31};	//Color brightness at 0 1 2 and 3 (2 bits)
static unsigned char ColorTable32Bit[4]={0,85,170,255};	
static unsigned short Afacts16[2][4]={0,0xF800,0x001F,0xFFFF,0,0x001F,0xF800,0xFFFF};
//static unsigned char Afacts8[2][4]={0,164,137,191,0,137,164,191};
static unsigned char Afacts8[2][4]={0,0xA4,0x89,0xBF,0,137,164,191};
static unsigned int Afacts32[2][4]={0,0xFF8D1F,0x0667FF,0xFFFFFF,0,0x0667FF,0xFF8D1F,0xFFFFFF};
static unsigned char  Pallete[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};		//Coco 3 6 bit colors
static unsigned char  Pallete8Bit[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned short Pallete16Bit[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	//Color values translated to 16bit 32BIT
static unsigned int   Pallete32Bit[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	//Color values translated to 24/32 bits

static unsigned int VidMask=0x1FFFF;
static unsigned char VresIndex=0;
static unsigned char CC2Offset=0,CC2VDGMode=0,CC2VDGPiaMode=0;
static unsigned short VerticalOffsetRegister=0;
static unsigned char CompatMode=0;
static unsigned char  PalleteLookup8[2][64];	//0 = RGB 1=comp 8BIT
static unsigned short PalleteLookup16[2][64];	//0 = RGB 1=comp 16BIT
static unsigned int   PalleteLookup32[2][64];	//0 = RGB 1=comp 32BIT
static unsigned char MonType=1;
static unsigned char CC3Vmode=0,CC3Vres=0,CC3BoarderColor=0;
static unsigned int StartofVidram=0,Start=0,NewStartofVidram=0;
static unsigned char LinesperScreen=0;
static unsigned char Bpp=0;
static unsigned char LinesperRow=1,BytesperRow=32;
static unsigned char GraphicsMode=0;
static unsigned char TextFGColor=0,TextBGColor=0;
static unsigned char TextFGPallete=0,TextBGPallete=0;
static unsigned char PalleteIndex=0;
static unsigned short PixelsperLine=0,VPitch=32;
static unsigned char Stretch=0,PixelsperByte=0;
static unsigned char HorzCenter=0,VertCenter=0;
static unsigned char LowerCase=0,InvertAll=0,ExtendedText=1;
static unsigned char HorzOffsetReg=0;
static unsigned char Hoffset=0;
static unsigned short TagY=0;
static unsigned short HorzBeam=0;
static unsigned int BoarderColor32=0;
static unsigned short BoarderColor16=0;
static unsigned char BoarderColor8=0;
static unsigned int DistoOffset=0;
static unsigned char BoarderChange=3;
static unsigned char MasterMode=0;
static unsigned char ColorInvert=1;
static unsigned char BlinkState=1;

// BEGIN of 8 Bit render loop *****************************************************************************************
void UpdateScreen8 (SystemState *US8State)
{
	register unsigned int YStride=0;
	unsigned char Pixel=0;
	unsigned char Character=0,Attributes=0;
	unsigned char TextPallete[2]={0,0};
	unsigned short WidePixel=0;
	char Pix=0,Bit=0,Sphase=0;
	static char Carry1=0,Carry2=0;
	static char Pcolor=0;
	unsigned char *buffer=US8State->RamBuffer;
	Carry1=1;
	Pcolor=0;
	
	if ( (HorzCenter!=0) & (BoarderChange>0) )
		for (unsigned short x=0;x<HorzCenter;x++)
		{
			US8State->PTRsurface8[x +(((US8State->LineCounter+VertCenter)*2)*US8State->SurfacePitch)]=BoarderColor8;
			if (!US8State->ScanLines)
				US8State->PTRsurface8[x +(((US8State->LineCounter+VertCenter)*2+1)*US8State->SurfacePitch)]=BoarderColor8;
			US8State->PTRsurface8[x + (PixelsperLine * (Stretch+1)) +HorzCenter+(((US8State->LineCounter+VertCenter)*2)*US8State->SurfacePitch)]=BoarderColor8;
			if (!US8State->ScanLines)
				US8State->PTRsurface8[x + (PixelsperLine * (Stretch+1))+HorzCenter+(((US8State->LineCounter+VertCenter)*2+1)*US8State->SurfacePitch)]=BoarderColor8;
		}

	if (LinesperRow < 13)
		TagY++;
	
	if (!US8State->LineCounter)
	{
		StartofVidram=NewStartofVidram;
		TagY=US8State->LineCounter;
	}
	Start=StartofVidram+(TagY/LinesperRow)*(VPitch*ExtendedText);
	YStride=(((US8State->LineCounter+VertCenter)*2)*US8State->SurfacePitch)+(HorzCenter)-1;

	switch (MasterMode) // (GraphicsMode <<7) | (CompatMode<<6)  | ((Bpp & 3)<<4) | (Stretch & 15);
		{
			case 0: //Width 80
				Attributes=0;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
					Pixel=cc3Fontdata8x12[Character * 12 + (US8State->LineCounter%LinesperRow)]; 

					if (ExtendedText==2)
					{
						Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
						if  ( (Attributes & 64) && (US8State->LineCounter%LinesperRow==(LinesperRow-1)) )	//UnderLine
							Pixel=255;
						if ((!BlinkState) & !!(Attributes & 128))
							Pixel=0;
					}
					TextPallete[1]=Pallete8Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete8Bit[Attributes & 7];
					US8State->PTRsurface8[YStride+=1]=TextPallete[Pixel >>7 ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[Pixel&1];
					if (!US8State->ScanLines)
					{
						YStride-=(8);
						YStride+=US8State->SurfacePitch;
						US8State->PTRsurface8[YStride+=1]=TextPallete[Pixel >>7 ];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[Pixel&1];
						YStride-=US8State->SurfacePitch;
					}
				} 

			break;
			case 1:
			case 2: //Width 40
				Attributes=0;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
					Pixel=cc3Fontdata8x12[Character  * 12 + (US8State->LineCounter%LinesperRow)]; 
					if (ExtendedText==2)
					{
						Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
						if  ( (Attributes & 64) && (US8State->LineCounter%LinesperRow==(LinesperRow-1)) )	//UnderLine
							Pixel=255;
						if ((!BlinkState) & !!(Attributes & 128))
							Pixel=0;
					}
					TextPallete[1]=Pallete8Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete8Bit[Attributes & 7];
					US8State->PTRsurface8[YStride+=1]=TextPallete[Pixel>>7 ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[Pixel>>7 ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
					if (!US8State->ScanLines)
					{
						YStride-=(16);
						YStride+=US8State->SurfacePitch;
						US8State->PTRsurface8[YStride+=1]=TextPallete[Pixel>>7 ];
						US8State->PTRsurface8[YStride+=1]=TextPallete[Pixel>>7 ];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
						YStride-=US8State->SurfacePitch;
					}
				} 
			break;
//			case 0:		//Hi Res text GraphicsMode=0 CompatMode=0 Ignore Bpp and Stretch

		//	case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:		
			case 17:		
			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			case 32:		
			case 33:			
			case 34:
			case 35:
			case 36:
			case 37:
			case 38:
			case 39:
			case 40:
			case 41:
			case 42:
			case 43:
			case 44:
			case 45:
			case 46:
			case 47:
			case 48:
			case 49:
			case 50:
			case 51:
			case 52:
			case 53:
			case 54:
			case 55:
			case 56:
			case 57:
			case 58:
			case 59:
			case 60:
			case 61:
			case 62:
			case 63:
				return;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
					if (ExtendedText==2)
						Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
					else
						Attributes=0;
					Pixel=cc3Fontdata8x12[(Character & 127) * 8 + (US8State->LineCounter%8)]; 
					if  ( (Attributes & 64) && (US8State->LineCounter%8==7) )	//UnderLine
						Pixel=255;
					if ((!BlinkState) & !!(Attributes & 128))
						Pixel=0;				
					TextPallete[1]=Pallete8Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete8Bit[Attributes & 7];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 128)/128 ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 64)/64];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 32)/32 ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 16)/16];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 8)/8 ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 4)/4];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 2)/2 ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
				} 
			break;

//******************************************************************** Low Res Text
			case 64:		//Low Res text GraphicsMode=0 CompatMode=1 Ignore Bpp and Stretch
			case 65:
			case 66:
			case 67:
			case 68:
			case 69:
			case 70:
			case 71:
			case 72:
			case 73:
			case 74:
			case 75:
			case 76:
			case 77:
			case 78:
			case 79:
			case 80:		
			case 81:		
			case 82:
			case 83:
			case 84:
			case 85:
			case 86:
			case 87:
			case 88:
			case 89:
			case 90:
			case 91:
			case 92:
			case 93:
			case 94:
			case 95:
			case 96:		
			case 97:	
			case 98:
			case 99:
			case 100:
			case 101:
			case 102:
			case 103:
			case 104:
			case 105:
			case 106:
			case 107:
			case 108:
			case 109:
			case 110:
			case 111:
			case 112:		
			case 113:			
			case 114:
			case 115:
			case 116:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 123:
			case 124:
			case 125:
			case 126:
			case 127:

				for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam++)
				{										
					Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
					switch ((Character & 192) >> 6)
					{
					case 0:
						Character = Character & 63;
						TextPallete[0]=Pallete8Bit[TextBGPallete];
						TextPallete[1]=Pallete8Bit[TextFGPallete];
						if (LowerCase & (Character < 32))
							Pixel=ntsc_round_fontdata8x12[(Character + 80 )*12+ (US8State->LineCounter%12)];
						else
							Pixel=~ntsc_round_fontdata8x12[(Character )*12+ (US8State->LineCounter%12)];
					break;

					case 1:
						Character = Character & 63;
						TextPallete[0]=Pallete8Bit[TextBGPallete];
						TextPallete[1]=Pallete8Bit[TextFGPallete];
						Pixel=ntsc_round_fontdata8x12[(Character )*12+ (US8State->LineCounter%12)];
					break;

					case 2:
					case 3:
						TextPallete[1] = Pallete8Bit[(Character & 112) >> 4];
						TextPallete[0] = Pallete8Bit[8];
						Character = 64 + (Character & 0xF);
						Pixel=ntsc_round_fontdata8x12[(Character )*12+ (US8State->LineCounter%12)];
					break;

					} //END SWITCH
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>7) ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>7) ];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
					US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
					if (!US8State->ScanLines)
					{
						YStride-=(16);
						YStride+=US8State->SurfacePitch;
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>7) ];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>7) ];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>6)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>5)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>4)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>3)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>2)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel>>1)&1];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
						US8State->PTRsurface8[YStride+=1]=TextPallete[(Pixel & 1)];
						YStride-=US8State->SurfacePitch;
					}
				
				} // Next HorzBeam

			break;

case 128+0: //Bpp=0 Sr=0 1BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=1
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		if (!US8State->ScanLines)
		{
			YStride-=(16);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
	}
}
break;
//case 192+1:
case 128+1: //Bpp=0 Sr=1 1BPP Stretch=2
case 128+2:	//Bpp=0 Sr=2 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=2
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];

		if (!US8State->ScanLines)
		{
			YStride-=(32);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
	}
}
	break;

case 128+3: //Bpp=0 Sr=3 1BPP Stretch=4
case 128+4: //Bpp=0 Sr=4
case 128+5: //Bpp=0 Sr=5
case 128+6: //Bpp=0 Sr=6
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=4
{
	WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];

	if (!US8State->ScanLines)
	{
		YStride-=(64);
		YStride+=US8State->SurfacePitch;
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		YStride-=US8State->SurfacePitch;
	}
}
break;
case 128+7: //Bpp=0 Sr=7 1BPP Stretch=8 
case 128+8: //Bpp=0 Sr=8
case 128+9: //Bpp=0 Sr=9
case 128+10: //Bpp=0 Sr=10
case 128+11: //Bpp=0 Sr=11
case 128+12: //Bpp=0 Sr=12
case 128+13: //Bpp=0 Sr=13
case 128+14: //Bpp=0 Sr=14
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=8
{
	WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];

	if (!US8State->ScanLines)
	{
		YStride-=(128);
		YStride+=US8State->SurfacePitch;
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>7)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>5)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>3)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>1)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>15)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>13)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>11)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>9)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 1 & (WidePixel>>8)];
		YStride-=US8State->SurfacePitch;
	}
}
break;

case 128+15: //Bpp=0 Sr=15 1BPP Stretch=16
case 128+16: //BPP=1 Sr=0  2BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=1
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		if (!US8State->ScanLines)
		{
			YStride-=(8);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 128+17: //Bpp=1 Sr=1  2BPP Stretch=2
case 128+18: //Bpp=1 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=2
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];

		if (!US8State->ScanLines)
		{
			YStride-=(16);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 128+19: //Bpp=1 Sr=3  2BPP Stretch=4
case 128+20: //Bpp=1 Sr=4
case 128+21: //Bpp=1 Sr=5
case 128+22: //Bpp=1 Sr=6
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=4
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];

		if (!US8State->ScanLines)
		{
			YStride-=(32);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 128+23: //Bpp=1 Sr=7  2BPP Stretch=8
case 128+24: //Bpp=1 Sr=8
case 128+25: //Bpp=1 Sr=9 
case 128+26: //Bpp=1 Sr=10 
case 128+27: //Bpp=1 Sr=11 
case 128+28: //Bpp=1 Sr=12 
case 128+29: //Bpp=1 Sr=13 
case 128+30: //Bpp=1 Sr=14
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=8
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];

		if (!US8State->ScanLines)
		{
			YStride-=(64);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;
	
case 128+31: //Bpp=1 Sr=15 2BPP Stretch=16 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=16
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];

		if (!US8State->ScanLines)
		{
			YStride-=(128);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>6)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>2)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>14)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>10)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 3 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 128+32: //Bpp=2 Sr=0 4BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=1
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		if (!US8State->ScanLines)
		{
			YStride-=(4);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 128+33: //Bpp=2 Sr=1 4BPP Stretch=2 
case 128+34: //Bpp=2 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=2
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		if (!US8State->ScanLines)
		{
			YStride-=(8);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 128+35: //Bpp=2 Sr=3 4BPP Stretch=4
case 128+36: //Bpp=2 Sr=4 
case 128+37: //Bpp=2 Sr=5 
case 128+38: //Bpp=2 Sr=6 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=4
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];

		if (!US8State->ScanLines)
		{
			YStride-=(16);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 128+39: //Bpp=2 Sr=7 4BPP Stretch=8
case 128+40: //Bpp=2 Sr=8 
case 128+41: //Bpp=2 Sr=9 
case 128+42: //Bpp=2 Sr=10 
case 128+43: //Bpp=2 Sr=11 
case 128+44: //Bpp=2 Sr=12 
case 128+45: //Bpp=2 Sr=13 
case 128+46: //Bpp=2 Sr=14 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=8
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];

		if (!US8State->ScanLines)
		{
			YStride-=(32);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 128+47: //Bpp=2 Sr=15 4BPP Stretch=16
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=16
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];

		if (!US8State->ScanLines)
		{
			YStride-=(64);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>4)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & WidePixel];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>12)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[ 15 & (WidePixel>>8)];
			YStride-=US8State->SurfacePitch;
		}
	}
break;	
case 128+48: //Bpp=3 Sr=0  Unsupported 
case 128+49: //Bpp=3 Sr=1 
case 128+50: //Bpp=3 Sr=2 
case 128+51: //Bpp=3 Sr=3 
case 128+52: //Bpp=3 Sr=4 
case 128+53: //Bpp=3 Sr=5 
case 128+54: //Bpp=3 Sr=6 
case 128+55: //Bpp=3 Sr=7 
case 128+56: //Bpp=3 Sr=8 
case 128+57: //Bpp=3 Sr=9 
case 128+58: //Bpp=3 Sr=10 
case 128+59: //Bpp=3 Sr=11 
case 128+60: //Bpp=3 Sr=12 
case 128+61: //Bpp=3 Sr=13 
case 128+62: //Bpp=3 Sr=14 
case 128+63: //Bpp=3 Sr=15 

	return;
	break;

case 192+0: //Bpp=0 Sr=0 1BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=1
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		if (!US8State->ScanLines)
		{
			YStride-=(16);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			YStride-=US8State->SurfacePitch;
	}
}
break;

case 192+1: //Bpp=0 Sr=1 1BPP Stretch=2
case 192+2:	//Bpp=0 Sr=2 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=2
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
//************************************************************************************
		if (!MonType)
		{ //Pcolor
			for (Bit=7;Bit>=0;Bit--)
			{
				Pix=(1 & (WidePixel>>Bit) );
				Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
				switch(Sphase)
				{
				case 0:
				case 4:
				case 6:
					Pcolor=0;
					break;
				case 1:
				case 5:
					Pcolor=(Bit &1)+1;
					break;
				case 2:
				//	Pcolor=(!(Bit &1))+1; Use last color
					break;
				case 3:
					Pcolor=3;
					US8State->PTRsurface8[YStride-1]=Afacts8[ColorInvert][3];
					if (!US8State->ScanLines)
						US8State->PTRsurface8[YStride+US8State->SurfacePitch-1]=Afacts8[ColorInvert][3];
					US8State->PTRsurface8[YStride]=Afacts8[ColorInvert][3];
					if (!US8State->ScanLines)
						US8State->PTRsurface8[YStride+US8State->SurfacePitch]=Afacts8[ColorInvert][3];
					break;
				case 7:
					Pcolor=3;
					break;
				} //END Switch

				US8State->PTRsurface8[YStride+=1]=Afacts8[ColorInvert][Pcolor];
				if (!US8State->ScanLines)
					US8State->PTRsurface8[YStride+US8State->SurfacePitch]=Afacts8[ColorInvert][Pcolor];
				US8State->PTRsurface8[YStride+=1]=Afacts8[ColorInvert][Pcolor];
				if (!US8State->ScanLines)
					US8State->PTRsurface8[YStride+US8State->SurfacePitch]=Afacts8[ColorInvert][Pcolor];
				Carry2=Carry1;
				Carry1=Pix;
			}

			for (Bit=15;Bit>=8;Bit--)
			{
				Pix=(1 & (WidePixel>>Bit) );
				Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
				switch(Sphase)
				{
				case 0:
				case 4:
				case 6:
					Pcolor=0;
					break;
				case 1:
				case 5:
					Pcolor=(Bit &1)+1;
					break;
				case 2:
				//	Pcolor=(!(Bit &1))+1; Use last color
					break;
				case 3:
					Pcolor=3;
					US8State->PTRsurface8[YStride-1]=Afacts8[ColorInvert][3];
					if (!US8State->ScanLines)
						US8State->PTRsurface8[YStride+US8State->SurfacePitch-1]=Afacts8[ColorInvert][3];
					US8State->PTRsurface8[YStride]=Afacts8[ColorInvert][3];
					if (!US8State->ScanLines)
						US8State->PTRsurface8[YStride+US8State->SurfacePitch]=Afacts8[ColorInvert][3];
					break;
				case 7:
					Pcolor=3;
					break;
				} //END Switch

				US8State->PTRsurface8[YStride+=1]=Afacts8[ColorInvert][Pcolor];
				if (!US8State->ScanLines)
					US8State->PTRsurface8[YStride+US8State->SurfacePitch]=Afacts8[ColorInvert][Pcolor];
				US8State->PTRsurface8[YStride+=1]=Afacts8[ColorInvert][Pcolor];
				if (!US8State->ScanLines)
					US8State->PTRsurface8[YStride+US8State->SurfacePitch]=Afacts8[ColorInvert][Pcolor];
				Carry2=Carry1;
				Carry1=Pix;
			}

		}
			else
			{
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			if (!US8State->ScanLines)
			{
				YStride-=(32);
				YStride+=US8State->SurfacePitch;
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				YStride-=US8State->SurfacePitch;
			}
		}

}
	break;

case 192+3: //Bpp=0 Sr=3 1BPP Stretch=4
case 192+4: //Bpp=0 Sr=4
case 192+5: //Bpp=0 Sr=5
case 192+6: //Bpp=0 Sr=6
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=4
{
	WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];

	if (!US8State->ScanLines)
	{
		YStride-=(64);
		YStride+=US8State->SurfacePitch;
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		YStride-=US8State->SurfacePitch;
	}
}
break;
case 192+7: //Bpp=0 Sr=7 1BPP Stretch=8 
case 192+8: //Bpp=0 Sr=8
case 192+9: //Bpp=0 Sr=9
case 192+10: //Bpp=0 Sr=10
case 192+11: //Bpp=0 Sr=11
case 192+12: //Bpp=0 Sr=12
case 192+13: //Bpp=0 Sr=13
case 192+14: //Bpp=0 Sr=14
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=8
{
	WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];

	if (!US8State->ScanLines)
	{
		YStride-=(128);
		YStride+=US8State->SurfacePitch;
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		YStride-=US8State->SurfacePitch;
	}
}
break;

case 192+15: //Bpp=0 Sr=15 1BPP Stretch=16
case 192+16: //BPP=1 Sr=0  2BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=1
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		if (!US8State->ScanLines)
		{
			YStride-=(8);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 192+17: //Bpp=1 Sr=1  2BPP Stretch=2
case 192+18: //Bpp=1 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=2
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!US8State->ScanLines)
		{
			YStride-=(16);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 192+19: //Bpp=1 Sr=3  2BPP Stretch=4
case 192+20: //Bpp=1 Sr=4
case 192+21: //Bpp=1 Sr=5
case 192+22: //Bpp=1 Sr=6
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=4
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!US8State->ScanLines)
		{
			YStride-=(32);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 192+23: //Bpp=1 Sr=7  2BPP Stretch=8
case 192+24: //Bpp=1 Sr=8
case 192+25: //Bpp=1 Sr=9 
case 192+26: //Bpp=1 Sr=10 
case 192+27: //Bpp=1 Sr=11 
case 192+28: //Bpp=1 Sr=12 
case 192+29: //Bpp=1 Sr=13 
case 192+30: //Bpp=1 Sr=14
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=8
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!US8State->ScanLines)
		{
			YStride-=(64);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=US8State->SurfacePitch;
		}
	}
break;
	
case 192+31: //Bpp=1 Sr=15 2BPP Stretch=16 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=16
	{
		WidePixel=US8State->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!US8State->ScanLines)
		{
			YStride-=(128);
			YStride+=US8State->SurfacePitch;
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & WidePixel)];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			US8State->PTRsurface8[YStride+=1]=Pallete8Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=US8State->SurfacePitch;
		}
	}
break;

case 192+32: //Bpp=2 Sr=0 4BPP Stretch=1 Unsupport with Compat set
case 192+33: //Bpp=2 Sr=1 4BPP Stretch=2 
case 192+34: //Bpp=2 Sr=2
case 192+35: //Bpp=2 Sr=3 4BPP Stretch=4
case 192+36: //Bpp=2 Sr=4 
case 192+37: //Bpp=2 Sr=5 
case 192+38: //Bpp=2 Sr=6 
case 192+39: //Bpp=2 Sr=7 4BPP Stretch=8
case 192+40: //Bpp=2 Sr=8 
case 192+41: //Bpp=2 Sr=9 
case 192+42: //Bpp=2 Sr=10 
case 192+43: //Bpp=2 Sr=11 
case 192+44: //Bpp=2 Sr=12 
case 192+45: //Bpp=2 Sr=13 
case 192+46: //Bpp=2 Sr=14 
case 192+47: //Bpp=2 Sr=15 4BPP Stretch=16
case 192+48: //Bpp=3 Sr=0  Unsupported 
case 192+49: //Bpp=3 Sr=1 
case 192+50: //Bpp=3 Sr=2 
case 192+51: //Bpp=3 Sr=3 
case 192+52: //Bpp=3 Sr=4 
case 192+53: //Bpp=3 Sr=5 
case 192+54: //Bpp=3 Sr=6 
case 192+55: //Bpp=3 Sr=7 
case 192+56: //Bpp=3 Sr=8 
case 192+57: //Bpp=3 Sr=9 
case 192+58: //Bpp=3 Sr=10 
case 192+59: //Bpp=3 Sr=11 
case 192+60: //Bpp=3 Sr=12 
case 192+61: //Bpp=3 Sr=13 
case 192+62: //Bpp=3 Sr=14 
case 192+63: //Bpp=3 Sr=15 
	return;
	break;

			} //END SWITCH
	return;
}
// END of 8 Bit render loop *****************************************************************************************


// BEGIN of 16 Bit render loop *****************************************************************************************
void UpdateScreen16 (SystemState *USState16)
{
	register unsigned int YStride=0;
	static unsigned int TextColor=0;
	static unsigned char Pixel=0;
	static unsigned char Character=0,Attributes=0;
	static unsigned short TextPallete[2]={0,0};
	static unsigned short WidePixel=0;
	static char Pix=0,Bit=0,Sphase=0;
	static char Carry2=0;
	char Pcolor=0;
	char Carry1=1;

	if ( (HorzCenter!=0) & (BoarderChange>0) )
		for (unsigned short x=0;x<HorzCenter;x++)
		{
			USState16->PTRsurface16[x +(((USState16->LineCounter+VertCenter)*2)*(USState16->SurfacePitch))]=BoarderColor16;
			if (!USState16->ScanLines)
				USState16->PTRsurface16[x +(((USState16->LineCounter+VertCenter)*2+1)*(USState16->SurfacePitch))]=BoarderColor16;
			USState16->PTRsurface16[x + (PixelsperLine * (Stretch+1)) +HorzCenter+(((USState16->LineCounter+VertCenter)*2)*(USState16->SurfacePitch))]=BoarderColor16;
			if (!USState16->ScanLines)
				USState16->PTRsurface16[x + (PixelsperLine * (Stretch+1))+HorzCenter+(((USState16->LineCounter+VertCenter)*2+1)*(USState16->SurfacePitch))]=BoarderColor16;
		}

	if (LinesperRow < 13)
		TagY++;
	
	if (!USState16->LineCounter)
	{
		StartofVidram=NewStartofVidram;
		TagY=USState16->LineCounter;
	}
	Start=StartofVidram+(TagY/LinesperRow)*(VPitch*ExtendedText);
	YStride=(((USState16->LineCounter+VertCenter)*2)*USState16->SurfacePitch)+(HorzCenter*1)-1;

	switch (MasterMode) // (GraphicsMode <<7) | (CompatMode<<6)  | ((Bpp & 3)<<4) | (Stretch & 15);
		{
			case 0: //Width 80
				Attributes=0;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=USState16->RamBuffer[VidMask & (Start+(unsigned char)(HorzBeam+Hoffset))];
					Pixel=cc3Fontdata8x12[Character * 12 + (USState16->LineCounter%LinesperRow)]; 

					if (ExtendedText==2)
					{
						Attributes=USState16->RamBuffer[VidMask &(Start+(unsigned char)(HorzBeam+Hoffset)+1)];
						if  ( (Attributes & 64) && (USState16->LineCounter%LinesperRow==(LinesperRow-1)) )	//UnderLine
							Pixel=255;
						if ((!BlinkState) & !!(Attributes & 128))
							Pixel=0;
					}
					TextPallete[1]=Pallete16Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete16Bit[Attributes & 7];
					USState16->PTRsurface16[YStride+=1]=TextPallete[Pixel >>7 ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[Pixel&1];
					if (!USState16->ScanLines)
					{
						YStride-=(8);
						YStride+=USState16->SurfacePitch;
						USState16->PTRsurface16[YStride+=1]=TextPallete[Pixel >>7 ];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[Pixel&1];
						YStride-=USState16->SurfacePitch;
					}
				} 

			break;
			case 1:
			case 2: //Width 40
				Attributes=0;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=USState16->RamBuffer[VidMask &(Start+(unsigned char)(HorzBeam+Hoffset))];
					Pixel=cc3Fontdata8x12[Character  * 12 + (USState16->LineCounter%LinesperRow)]; 
					if (ExtendedText==2)
					{
						Attributes=USState16->RamBuffer[VidMask &(Start+(unsigned char)(HorzBeam+Hoffset)+1)];
						if  ( (Attributes & 64) && (USState16->LineCounter%LinesperRow==(LinesperRow-1)) )	//UnderLine
							Pixel=255;
						if ((!BlinkState) & !!(Attributes & 128))
							Pixel=0;
					}
					TextPallete[1]=Pallete16Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete16Bit[Attributes & 7];
					USState16->PTRsurface16[YStride+=1]=TextPallete[Pixel>>7 ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[Pixel>>7 ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
					if (!USState16->ScanLines)
					{
						YStride-=(16);
						YStride+=USState16->SurfacePitch;
						USState16->PTRsurface16[YStride+=1]=TextPallete[Pixel>>7 ];
						USState16->PTRsurface16[YStride+=1]=TextPallete[Pixel>>7 ];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
						YStride-=USState16->SurfacePitch;
					}
				} 
			break;
//			case 0:		//Hi Res text GraphicsMode=0 CompatMode=0 Ignore Bpp and Stretch

		//	case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:		
			case 17:		
			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			case 32:		
			case 33:			
			case 34:
			case 35:
			case 36:
			case 37:
			case 38:
			case 39:
			case 40:
			case 41:
			case 42:
			case 43:
			case 44:
			case 45:
			case 46:
			case 47:
			case 48:
			case 49:
			case 50:
			case 51:
			case 52:
			case 53:
			case 54:
			case 55:
			case 56:
			case 57:
			case 58:
			case 59:
			case 60:
			case 61:
			case 62:
			case 63:
				return;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=USState16->RamBuffer[VidMask & (Start+(unsigned char)(HorzBeam+Hoffset))];
					if (ExtendedText==2)
						Attributes=USState16->RamBuffer[VidMask &(Start+(unsigned char)(HorzBeam+Hoffset)+1)];
					else
						Attributes=0;
					Pixel=cc3Fontdata8x12[(Character & 127) * 8 + (USState16->LineCounter%8)]; 
					if  ( (Attributes & 64) && (USState16->LineCounter%8==7) )	//UnderLine
						Pixel=255;
					if ((!BlinkState) & !!(Attributes & 128))
						Pixel=0;				
					TextPallete[1]=Pallete16Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete16Bit[Attributes & 7];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 128)/128 ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 64)/64];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 32)/32 ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 16)/16];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 8)/8 ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 4)/4];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 2)/2 ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
				} 
			break;

//******************************************************************** Low Res Text
			case 64:		//Low Res text GraphicsMode=0 CompatMode=1 Ignore Bpp and Stretch
			case 65:
			case 66:
			case 67:
			case 68:
			case 69:
			case 70:
			case 71:
			case 72:
			case 73:
			case 74:
			case 75:
			case 76:
			case 77:
			case 78:
			case 79:
			case 80:		
			case 81:		
			case 82:
			case 83:
			case 84:
			case 85:
			case 86:
			case 87:
			case 88:
			case 89:
			case 90:
			case 91:
			case 92:
			case 93:
			case 94:
			case 95:
			case 96:		
			case 97:	
			case 98:
			case 99:
			case 100:
			case 101:
			case 102:
			case 103:
			case 104:
			case 105:
			case 106:
			case 107:
			case 108:
			case 109:
			case 110:
			case 111:
			case 112:		
			case 113:			
			case 114:
			case 115:
			case 116:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 123:
			case 124:
			case 125:
			case 126:
			case 127:

				for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam++)
				{										
					Character=USState16->RamBuffer[VidMask & (Start+(unsigned char)(HorzBeam+Hoffset))];
					switch ((Character & 192) >> 6)
					{
					case 0:
						Character = Character & 63;
						TextPallete[0]=Pallete16Bit[TextBGPallete];
						TextPallete[1]=Pallete16Bit[TextFGPallete];
						if (LowerCase & (Character < 32))
							Pixel=ntsc_round_fontdata8x12[(Character + 80 )*12+ (USState16->LineCounter%12)];
						else
							Pixel=~ntsc_round_fontdata8x12[(Character )*12+ (USState16->LineCounter%12)];
					break;

					case 1:
						Character = Character & 63;
						TextPallete[0]=Pallete16Bit[TextBGPallete];
						TextPallete[1]=Pallete16Bit[TextFGPallete];
						Pixel=ntsc_round_fontdata8x12[(Character )*12+ (USState16->LineCounter%12)];
					break;

					case 2:
					case 3:
						TextPallete[1] = Pallete16Bit[(Character & 112) >> 4];
						TextPallete[0] = Pallete16Bit[8];
						Character = 64 + (Character & 0xF);
						Pixel=ntsc_round_fontdata8x12[(Character )*12+ (USState16->LineCounter%12)];
					break;

					} //END SWITCH
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>7) ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>7) ];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
					USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
					if (!USState16->ScanLines)
					{
						YStride-=(16);
						YStride+=USState16->SurfacePitch;
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>7) ];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>7) ];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>6)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>5)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>4)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>3)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>2)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel>>1)&1];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
						USState16->PTRsurface16[YStride+=1]=TextPallete[(Pixel & 1)];
						YStride-=USState16->SurfacePitch;
					}
				
				} // Next HorzBeam

			break;
case 128+0: //Bpp=0 Sr=0 1BPP Stretch=1

	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=1
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		if (!USState16->ScanLines)
		{
			YStride-=(16);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
	}
}
break;
//case 192+1:
case 128+1: //Bpp=0 Sr=1 1BPP Stretch=2
case 128+2:	//Bpp=0 Sr=2 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=2
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];

		if (!USState16->ScanLines)
		{
			YStride-=(32);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
	}
}
	break;

case 128+3: //Bpp=0 Sr=3 1BPP Stretch=4
case 128+4: //Bpp=0 Sr=4
case 128+5: //Bpp=0 Sr=5
case 128+6: //Bpp=0 Sr=6
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=4
{
	WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];

	if (!USState16->ScanLines)
	{
		YStride-=(64);
		YStride+=USState16->SurfacePitch;
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		YStride-=USState16->SurfacePitch;
	}
}
break;
case 128+7: //Bpp=0 Sr=7 1BPP Stretch=8 
case 128+8: //Bpp=0 Sr=8
case 128+9: //Bpp=0 Sr=9
case 128+10: //Bpp=0 Sr=10
case 128+11: //Bpp=0 Sr=11
case 128+12: //Bpp=0 Sr=12
case 128+13: //Bpp=0 Sr=13
case 128+14: //Bpp=0 Sr=14
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=8
{
	WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];

	if (!USState16->ScanLines)
	{
		YStride-=(128);
		YStride+=USState16->SurfacePitch;
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>7)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>5)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>3)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>1)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>15)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>13)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>11)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>9)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 1 & (WidePixel>>8)];
		YStride-=USState16->SurfacePitch;
	}
}
break;

case 128+15: //Bpp=0 Sr=15 1BPP Stretch=16
case 128+16: //BPP=1 Sr=0  2BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=1
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		if (!USState16->ScanLines)
		{
			YStride-=(8);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 128+17: //Bpp=1 Sr=1  2BPP Stretch=2
case 128+18: //Bpp=1 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=2
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];

		if (!USState16->ScanLines)
		{
			YStride-=(16);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 128+19: //Bpp=1 Sr=3  2BPP Stretch=4
case 128+20: //Bpp=1 Sr=4
case 128+21: //Bpp=1 Sr=5
case 128+22: //Bpp=1 Sr=6
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=4
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];

		if (!USState16->ScanLines)
		{
			YStride-=(32);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 128+23: //Bpp=1 Sr=7  2BPP Stretch=8
case 128+24: //Bpp=1 Sr=8
case 128+25: //Bpp=1 Sr=9 
case 128+26: //Bpp=1 Sr=10 
case 128+27: //Bpp=1 Sr=11 
case 128+28: //Bpp=1 Sr=12 
case 128+29: //Bpp=1 Sr=13 
case 128+30: //Bpp=1 Sr=14
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=8
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];

		if (!USState16->ScanLines)
		{
			YStride-=(64);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;
	
case 128+31: //Bpp=1 Sr=15 2BPP Stretch=16 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=16
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];

		if (!USState16->ScanLines)
		{
			YStride-=(128);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>6)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>2)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>14)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>10)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 3 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 128+32: //Bpp=2 Sr=0 4BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=1
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		if (!USState16->ScanLines)
		{
			YStride-=(4);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 128+33: //Bpp=2 Sr=1 4BPP Stretch=2 
case 128+34: //Bpp=2 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=2
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		if (!USState16->ScanLines)
		{
			YStride-=(8);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 128+35: //Bpp=2 Sr=3 4BPP Stretch=4
case 128+36: //Bpp=2 Sr=4 
case 128+37: //Bpp=2 Sr=5 
case 128+38: //Bpp=2 Sr=6 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=4
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];

		if (!USState16->ScanLines)
		{
			YStride-=(16);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 128+39: //Bpp=2 Sr=7 4BPP Stretch=8
case 128+40: //Bpp=2 Sr=8 
case 128+41: //Bpp=2 Sr=9 
case 128+42: //Bpp=2 Sr=10 
case 128+43: //Bpp=2 Sr=11 
case 128+44: //Bpp=2 Sr=12 
case 128+45: //Bpp=2 Sr=13 
case 128+46: //Bpp=2 Sr=14 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=8
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];

		if (!USState16->ScanLines)
		{
			YStride-=(32);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 128+47: //Bpp=2 Sr=15 4BPP Stretch=16
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=16
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];

		if (!USState16->ScanLines)
		{
			YStride-=(64);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>4)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & WidePixel];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>12)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[ 15 & (WidePixel>>8)];
			YStride-=USState16->SurfacePitch;
		}
	}
break;	
case 128+48: //Bpp=3 Sr=0  Unsupported 
case 128+49: //Bpp=3 Sr=1 
case 128+50: //Bpp=3 Sr=2 
case 128+51: //Bpp=3 Sr=3 
case 128+52: //Bpp=3 Sr=4 
case 128+53: //Bpp=3 Sr=5 
case 128+54: //Bpp=3 Sr=6 
case 128+55: //Bpp=3 Sr=7 
case 128+56: //Bpp=3 Sr=8 
case 128+57: //Bpp=3 Sr=9 
case 128+58: //Bpp=3 Sr=10 
case 128+59: //Bpp=3 Sr=11 
case 128+60: //Bpp=3 Sr=12 
case 128+61: //Bpp=3 Sr=13 
case 128+62: //Bpp=3 Sr=14 
case 128+63: //Bpp=3 Sr=15 

	return;
	break;
//	XXXXXXXXXXXXXXXXXXXXXX;
case 192+0: //Bpp=0 Sr=0 1BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=1
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		if (!USState16->ScanLines)
		{
			YStride-=(16);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			YStride-=USState16->SurfacePitch;
	}
}
break;

case 192+1: //Bpp=0 Sr=1 1BPP Stretch=2
case 192+2:	//Bpp=0 Sr=2 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=2
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
//************************************************************************************
		if (!MonType)
		{ //Pcolor
			for (Bit=7;Bit>=0;Bit--)
			{
				Pix=(1 & (WidePixel>>Bit) );
				Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
				switch(Sphase)
				{
				case 0:
				case 4:
				case 6:
					Pcolor=0;
					break;
				case 1:
				case 5:
					Pcolor=(Bit &1)+1;
					break;
				case 2:
				//	Pcolor=(!(Bit &1))+1; Use last color
					break;
				case 3:
					Pcolor=3;
					USState16->PTRsurface16[YStride-1]=Afacts16[ColorInvert][3];
					if (!USState16->ScanLines)
						USState16->PTRsurface16[YStride+USState16->SurfacePitch-1]=Afacts16[ColorInvert][3];
					USState16->PTRsurface16[YStride]=Afacts16[ColorInvert][3];
					if (!USState16->ScanLines)
						USState16->PTRsurface16[YStride+USState16->SurfacePitch]=Afacts16[ColorInvert][3];
					break;
				case 7:
					Pcolor=3;
					break;
				} //END Switch

				USState16->PTRsurface16[YStride+=1]=Afacts16[ColorInvert][Pcolor];
				if (!USState16->ScanLines)
					USState16->PTRsurface16[YStride+USState16->SurfacePitch]=Afacts16[ColorInvert][Pcolor];
				USState16->PTRsurface16[YStride+=1]=Afacts16[ColorInvert][Pcolor];
				if (!USState16->ScanLines)
					USState16->PTRsurface16[YStride+USState16->SurfacePitch]=Afacts16[ColorInvert][Pcolor];
				Carry2=Carry1;
				Carry1=Pix;
			}

			for (Bit=15;Bit>=8;Bit--)
			{
				Pix=(1 & (WidePixel>>Bit) );
				Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
				switch(Sphase)
				{
				case 0:
				case 4:
				case 6:
					Pcolor=0;
					break;
				case 1:
				case 5:
					Pcolor=(Bit &1)+1;
					break;
				case 2:
				//	Pcolor=(!(Bit &1))+1; Use last color
					break;
				case 3:
					Pcolor=3;
					USState16->PTRsurface16[YStride-1]=Afacts16[ColorInvert][3];
					if (!USState16->ScanLines)
						USState16->PTRsurface16[YStride+USState16->SurfacePitch-1]=Afacts16[ColorInvert][3];
					USState16->PTRsurface16[YStride]=Afacts16[ColorInvert][3];
					if (!USState16->ScanLines)
						USState16->PTRsurface16[YStride+USState16->SurfacePitch]=Afacts16[ColorInvert][3];
					break;
				case 7:
					Pcolor=3;
					break;
				} //END Switch

				USState16->PTRsurface16[YStride+=1]=Afacts16[ColorInvert][Pcolor];
				if (!USState16->ScanLines)
					USState16->PTRsurface16[YStride+USState16->SurfacePitch]=Afacts16[ColorInvert][Pcolor];
				USState16->PTRsurface16[YStride+=1]=Afacts16[ColorInvert][Pcolor];
				if (!USState16->ScanLines)
					USState16->PTRsurface16[YStride+USState16->SurfacePitch]=Afacts16[ColorInvert][Pcolor];
				Carry2=Carry1;
				Carry1=Pix;
			}

		}
			else
			{
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			if (!USState16->ScanLines)
			{
				YStride-=(32);
				YStride+=USState16->SurfacePitch;
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				YStride-=USState16->SurfacePitch;
			}
		}

}
	break;

case 192+3: //Bpp=0 Sr=3 1BPP Stretch=4
case 192+4: //Bpp=0 Sr=4
case 192+5: //Bpp=0 Sr=5
case 192+6: //Bpp=0 Sr=6
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=4
{
	WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];

	if (!USState16->ScanLines)
	{
		YStride-=(64);
		YStride+=USState16->SurfacePitch;
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		YStride-=USState16->SurfacePitch;
	}
}
break;
case 192+7: //Bpp=0 Sr=7 1BPP Stretch=8 
case 192+8: //Bpp=0 Sr=8
case 192+9: //Bpp=0 Sr=9
case 192+10: //Bpp=0 Sr=10
case 192+11: //Bpp=0 Sr=11
case 192+12: //Bpp=0 Sr=12
case 192+13: //Bpp=0 Sr=13
case 192+14: //Bpp=0 Sr=14
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=8
{
	WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];

	if (!USState16->ScanLines)
	{
		YStride-=(128);
		YStride+=USState16->SurfacePitch;
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		YStride-=USState16->SurfacePitch;
	}
}
break;

case 192+15: //Bpp=0 Sr=15 1BPP Stretch=16
case 192+16: //BPP=1 Sr=0  2BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=1
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		if (!USState16->ScanLines)
		{
			YStride-=(8);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 192+17: //Bpp=1 Sr=1  2BPP Stretch=2
case 192+18: //Bpp=1 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=2
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!USState16->ScanLines)
		{
			YStride-=(16);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 192+19: //Bpp=1 Sr=3  2BPP Stretch=4
case 192+20: //Bpp=1 Sr=4
case 192+21: //Bpp=1 Sr=5
case 192+22: //Bpp=1 Sr=6
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=4
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!USState16->ScanLines)
		{
			YStride-=(32);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 192+23: //Bpp=1 Sr=7  2BPP Stretch=8
case 192+24: //Bpp=1 Sr=8
case 192+25: //Bpp=1 Sr=9 
case 192+26: //Bpp=1 Sr=10 
case 192+27: //Bpp=1 Sr=11 
case 192+28: //Bpp=1 Sr=12 
case 192+29: //Bpp=1 Sr=13 
case 192+30: //Bpp=1 Sr=14
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=8
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!USState16->ScanLines)
		{
			YStride-=(64);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=USState16->SurfacePitch;
		}
	}
break;
	
case 192+31: //Bpp=1 Sr=15 2BPP Stretch=16 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=16
	{
		WidePixel=USState16->WRamBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!USState16->ScanLines)
		{
			YStride-=(128);
			YStride+=USState16->SurfacePitch;
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & WidePixel)];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			USState16->PTRsurface16[YStride+=1]=Pallete16Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=USState16->SurfacePitch;
		}
	}
break;

case 192+32: //Bpp=2 Sr=0 4BPP Stretch=1 Unsupport with Compat set
case 192+33: //Bpp=2 Sr=1 4BPP Stretch=2 
case 192+34: //Bpp=2 Sr=2
case 192+35: //Bpp=2 Sr=3 4BPP Stretch=4
case 192+36: //Bpp=2 Sr=4 
case 192+37: //Bpp=2 Sr=5 
case 192+38: //Bpp=2 Sr=6 
case 192+39: //Bpp=2 Sr=7 4BPP Stretch=8
case 192+40: //Bpp=2 Sr=8 
case 192+41: //Bpp=2 Sr=9 
case 192+42: //Bpp=2 Sr=10 
case 192+43: //Bpp=2 Sr=11 
case 192+44: //Bpp=2 Sr=12 
case 192+45: //Bpp=2 Sr=13 
case 192+46: //Bpp=2 Sr=14 
case 192+47: //Bpp=2 Sr=15 4BPP Stretch=16
case 192+48: //Bpp=3 Sr=0  Unsupported 
case 192+49: //Bpp=3 Sr=1 
case 192+50: //Bpp=3 Sr=2 
case 192+51: //Bpp=3 Sr=3 
case 192+52: //Bpp=3 Sr=4 
case 192+53: //Bpp=3 Sr=5 
case 192+54: //Bpp=3 Sr=6 
case 192+55: //Bpp=3 Sr=7 
case 192+56: //Bpp=3 Sr=8 
case 192+57: //Bpp=3 Sr=9 
case 192+58: //Bpp=3 Sr=10 
case 192+59: //Bpp=3 Sr=11 
case 192+60: //Bpp=3 Sr=12 
case 192+61: //Bpp=3 Sr=13 
case 192+62: //Bpp=3 Sr=14 
case 192+63: //Bpp=3 Sr=15 
	return;
	break;

			} //END SWITCH
	return;
}
// END of 16 Bit render loop *****************************************************************************************



// BEGIN of 24 Bit render loop *****************************************************************************************
void UpdateScreen24 (SystemState *USState24)
{

	return;
}
// END of 24 Bit render loop *****************************************************************************************



// BEGIN of 32 Bit render loop *****************************************************************************************
void UpdateScreen32 (SystemState *USState32)
{
	register unsigned int YStride=0;
//	unsigned int TextColor=0;
	unsigned char Pixel=0;
//	unsigned char StretchCount=0;
//	unsigned char Mask=0,BitCount=0,Peek=0;
	unsigned char Character=0,Attributes=0;
	unsigned int TextPallete[2]={0,0};
	unsigned short * WideBuffer=(unsigned short *)USState32->RamBuffer;
	unsigned char *buffer=USState32->RamBuffer;
	unsigned short WidePixel=0;
//	unsigned short lColor=0;
//	unsigned short Yindex[4]={316,308,300,292};
	char Pix=0,Bit=0,Sphase=0;
	static char Carry1=0,Carry2=0;
	static char Pcolor=0;
	unsigned int *szSurface32=USState32->PTRsurface32;
	unsigned short y=USState32->LineCounter;
	long Xpitch=USState32->SurfacePitch;
	Carry1=1;
	Pcolor=0;
	
	if ( (HorzCenter!=0) & (BoarderChange>0) )
		for (unsigned short x=0;x<HorzCenter;x++)
		{
			szSurface32[x +(((y+VertCenter)*2)*Xpitch)]=BoarderColor32;
			if (!USState32->ScanLines)
				szSurface32[x +(((y+VertCenter)*2+1)*Xpitch)]=BoarderColor32;
			szSurface32[x + (PixelsperLine * (Stretch+1)) +HorzCenter+(((y+VertCenter)*2)*Xpitch)]=BoarderColor32;
			if (!USState32->ScanLines)
				szSurface32[x + (PixelsperLine * (Stretch+1))+HorzCenter+(((y+VertCenter)*2+1)*Xpitch)]=BoarderColor32;
		}

	if (LinesperRow < 13)
		TagY++;
	
	if (!y)
	{
		StartofVidram=NewStartofVidram;
		TagY=y;
	}
	Start=StartofVidram+(TagY/LinesperRow)*(VPitch*ExtendedText);
	YStride=(((y+VertCenter)*2)*Xpitch)+(HorzCenter*1)-1;

	switch (MasterMode) // (GraphicsMode <<7) | (CompatMode<<6)  | ((Bpp & 3)<<4) | (Stretch & 15);
		{
			case 0: //Width 80
				Attributes=0;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
					Pixel=cc3Fontdata8x12[Character * 12 + (y%LinesperRow)]; 

					if (ExtendedText==2)
					{
						Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
						if  ( (Attributes & 64) && (y%LinesperRow==(LinesperRow-1)) )	//UnderLine
							Pixel=255;
						if ((!BlinkState) & !!(Attributes & 128))
							Pixel=0;
					}
					TextPallete[1]=Pallete32Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete32Bit[Attributes & 7];
					szSurface32[YStride+=1]=TextPallete[Pixel >>7 ];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[Pixel&1];
					if (!USState32->ScanLines)
					{
						YStride-=(8);
						YStride+=Xpitch;
						szSurface32[YStride+=1]=TextPallete[Pixel >>7 ];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
						szSurface32[YStride+=1]=TextPallete[Pixel&1];
						YStride-=Xpitch;
					}
				} 

			break;
			case 1:
			case 2: //Width 40
				Attributes=0;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
					Pixel=cc3Fontdata8x12[Character  * 12 + (y%LinesperRow)]; 
					if (ExtendedText==2)
					{
						Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
						if  ( (Attributes & 64) && (y%LinesperRow==(LinesperRow-1)) )	//UnderLine
							Pixel=255;
						if ((!BlinkState) & !!(Attributes & 128))
							Pixel=0;
					}
					TextPallete[1]=Pallete32Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete32Bit[Attributes & 7];
					szSurface32[YStride+=1]=TextPallete[Pixel>>7 ];
					szSurface32[YStride+=1]=TextPallete[Pixel>>7 ];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
					if (!USState32->ScanLines)
					{
						YStride-=(16);
						YStride+=Xpitch;
						szSurface32[YStride+=1]=TextPallete[Pixel>>7 ];
						szSurface32[YStride+=1]=TextPallete[Pixel>>7 ];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
						szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
						YStride-=Xpitch;
					}
				} 
			break;
//			case 0:		//Hi Res text GraphicsMode=0 CompatMode=0 Ignore Bpp and Stretch

		//	case 2:
			case 3:
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
			case 16:		
			case 17:		
			case 18:
			case 19:
			case 20:
			case 21:
			case 22:
			case 23:
			case 24:
			case 25:
			case 26:
			case 27:
			case 28:
			case 29:
			case 30:
			case 31:
			case 32:		
			case 33:			
			case 34:
			case 35:
			case 36:
			case 37:
			case 38:
			case 39:
			case 40:
			case 41:
			case 42:
			case 43:
			case 44:
			case 45:
			case 46:
			case 47:
			case 48:
			case 49:
			case 50:
			case 51:
			case 52:
			case 53:
			case 54:
			case 55:
			case 56:
			case 57:
			case 58:
			case 59:
			case 60:
			case 61:
			case 62:
			case 63:
				return;
				for (HorzBeam=0;HorzBeam<BytesperRow*ExtendedText;HorzBeam+=ExtendedText)
				{									
					Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
					if (ExtendedText==2)
						Attributes=buffer[Start+(unsigned char)(HorzBeam+Hoffset)+1];
					else
						Attributes=0;
					Pixel=cc3Fontdata8x12[(Character & 127) * 8 + (y%8)]; 
					if  ( (Attributes & 64) && (y%8==7) )	//UnderLine
						Pixel=255;
					if ((!BlinkState) & !!(Attributes & 128))
						Pixel=0;				
					TextPallete[1]=Pallete32Bit[8+((Attributes & 56)>>3)];
					TextPallete[0]=Pallete32Bit[Attributes & 7];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 128)/128 ];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 64)/64];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 32)/32 ];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 16)/16];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 8)/8 ];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 4)/4];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 2)/2 ];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
				} 
			break;

//******************************************************************** Low Res Text
			case 64:		//Low Res text GraphicsMode=0 CompatMode=1 Ignore Bpp and Stretch
			case 65:
			case 66:
			case 67:
			case 68:
			case 69:
			case 70:
			case 71:
			case 72:
			case 73:
			case 74:
			case 75:
			case 76:
			case 77:
			case 78:
			case 79:
			case 80:		
			case 81:		
			case 82:
			case 83:
			case 84:
			case 85:
			case 86:
			case 87:
			case 88:
			case 89:
			case 90:
			case 91:
			case 92:
			case 93:
			case 94:
			case 95:
			case 96:		
			case 97:	
			case 98:
			case 99:
			case 100:
			case 101:
			case 102:
			case 103:
			case 104:
			case 105:
			case 106:
			case 107:
			case 108:
			case 109:
			case 110:
			case 111:
			case 112:		
			case 113:			
			case 114:
			case 115:
			case 116:
			case 117:
			case 118:
			case 119:
			case 120:
			case 121:
			case 122:
			case 123:
			case 124:
			case 125:
			case 126:
			case 127:

				for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam++)
				{										
					Character=buffer[Start+(unsigned char)(HorzBeam+Hoffset)];
					switch ((Character & 192) >> 6)
					{
					case 0:
						Character = Character & 63;
						TextPallete[0]=Pallete32Bit[TextBGPallete];
						TextPallete[1]=Pallete32Bit[TextFGPallete];
						if (LowerCase & (Character < 32))
							Pixel=ntsc_round_fontdata8x12[(Character + 80 )*12+ (y%12)];
						else
							Pixel=~ntsc_round_fontdata8x12[(Character )*12+ (y%12)];
					break;

					case 1:
						Character = Character & 63;
						TextPallete[0]=Pallete32Bit[TextBGPallete];
						TextPallete[1]=Pallete32Bit[TextFGPallete];
						Pixel=ntsc_round_fontdata8x12[(Character )*12+ (y%12)];
					break;

					case 2:
					case 3:
						TextPallete[1] = Pallete32Bit[(Character & 112) >> 4];
						TextPallete[0] = Pallete32Bit[8];
						Character = 64 + (Character & 0xF);
						Pixel=ntsc_round_fontdata8x12[(Character )*12+ (y%12)];
					break;

					} //END SWITCH
					szSurface32[YStride+=1]=TextPallete[(Pixel>>7) ];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>7) ];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
					szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
					if (!USState32->ScanLines)
					{
						YStride-=(16);
						YStride+=Xpitch;
						szSurface32[YStride+=1]=TextPallete[(Pixel>>7) ];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>7) ];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>6)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>5)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>4)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>3)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>2)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel>>1)&1];
						szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
						szSurface32[YStride+=1]=TextPallete[(Pixel & 1)];
						YStride-=Xpitch;
					}
				
				} // Next HorzBeam

			break;
case 128+0: //Bpp=0 Sr=0 1BPP Stretch=1

	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=1
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		if (!USState32->ScanLines)
		{
			YStride-=(16);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			YStride-=Xpitch;
	}
}
break;
//case 192+1:
case 128+1: //Bpp=0 Sr=1 1BPP Stretch=2
case 128+2:	//Bpp=0 Sr=2 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=2
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];

		if (!USState32->ScanLines)
		{
			YStride-=(32);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
			YStride-=Xpitch;
	}
}
	break;

case 128+3: //Bpp=0 Sr=3 1BPP Stretch=4
case 128+4: //Bpp=0 Sr=4
case 128+5: //Bpp=0 Sr=5
case 128+6: //Bpp=0 Sr=6
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=4
{
	WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];

	if (!USState32->ScanLines)
	{
		YStride-=(64);
		YStride+=Xpitch;
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		YStride-=Xpitch;
	}
}
break;
case 128+7: //Bpp=0 Sr=7 1BPP Stretch=8 
case 128+8: //Bpp=0 Sr=8
case 128+9: //Bpp=0 Sr=9
case 128+10: //Bpp=0 Sr=10
case 128+11: //Bpp=0 Sr=11
case 128+12: //Bpp=0 Sr=12
case 128+13: //Bpp=0 Sr=13
case 128+14: //Bpp=0 Sr=14
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=8
{
	WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
	szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];

	if (!USState32->ScanLines)
	{
		YStride-=(128);
		YStride+=Xpitch;
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>7)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>5)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>3)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>1)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>15)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>13)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>11)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>9)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 1 & (WidePixel>>8)];
		YStride-=Xpitch;
	}
}
break;

case 128+15: //Bpp=0 Sr=15 1BPP Stretch=16
case 128+16: //BPP=1 Sr=0  2BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=1
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		if (!USState32->ScanLines)
		{
			YStride-=(8);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;

case 128+17: //Bpp=1 Sr=1  2BPP Stretch=2
case 128+18: //Bpp=1 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=2
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];

		if (!USState32->ScanLines)
		{
			YStride-=(16);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;

case 128+19: //Bpp=1 Sr=3  2BPP Stretch=4
case 128+20: //Bpp=1 Sr=4
case 128+21: //Bpp=1 Sr=5
case 128+22: //Bpp=1 Sr=6
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=4
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];

		if (!USState32->ScanLines)
		{
			YStride-=(32);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;

case 128+23: //Bpp=1 Sr=7  2BPP Stretch=8
case 128+24: //Bpp=1 Sr=8
case 128+25: //Bpp=1 Sr=9 
case 128+26: //Bpp=1 Sr=10 
case 128+27: //Bpp=1 Sr=11 
case 128+28: //Bpp=1 Sr=12 
case 128+29: //Bpp=1 Sr=13 
case 128+30: //Bpp=1 Sr=14
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=8
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];

		if (!USState32->ScanLines)
		{
			YStride-=(64);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;
	
case 128+31: //Bpp=1 Sr=15 2BPP Stretch=16 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=16
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];

		if (!USState32->ScanLines)
		{
			YStride-=(128);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>6)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>2)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>14)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>10)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 3 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;

case 128+32: //Bpp=2 Sr=0 4BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=1
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		if (!USState32->ScanLines)
		{
			YStride-=(4);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;

case 128+33: //Bpp=2 Sr=1 4BPP Stretch=2 
case 128+34: //Bpp=2 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=2
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		if (!USState32->ScanLines)
		{
			YStride-=(8);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;

case 128+35: //Bpp=2 Sr=3 4BPP Stretch=4
case 128+36: //Bpp=2 Sr=4 
case 128+37: //Bpp=2 Sr=5 
case 128+38: //Bpp=2 Sr=6 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=4
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];

		if (!USState32->ScanLines)
		{
			YStride-=(16);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;

case 128+39: //Bpp=2 Sr=7 4BPP Stretch=8
case 128+40: //Bpp=2 Sr=8 
case 128+41: //Bpp=2 Sr=9 
case 128+42: //Bpp=2 Sr=10 
case 128+43: //Bpp=2 Sr=11 
case 128+44: //Bpp=2 Sr=12 
case 128+45: //Bpp=2 Sr=13 
case 128+46: //Bpp=2 Sr=14 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=8
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];

		if (!USState32->ScanLines)
		{
			YStride-=(32);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;

case 128+47: //Bpp=2 Sr=15 4BPP Stretch=16
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //4bbp Stretch=16
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
		szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];

		if (!USState32->ScanLines)
		{
			YStride-=(64);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>4)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & WidePixel];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>12)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			szSurface32[YStride+=1]=Pallete32Bit[ 15 & (WidePixel>>8)];
			YStride-=Xpitch;
		}
	}
break;	
case 128+48: //Bpp=3 Sr=0  Unsupported 
case 128+49: //Bpp=3 Sr=1 
case 128+50: //Bpp=3 Sr=2 
case 128+51: //Bpp=3 Sr=3 
case 128+52: //Bpp=3 Sr=4 
case 128+53: //Bpp=3 Sr=5 
case 128+54: //Bpp=3 Sr=6 
case 128+55: //Bpp=3 Sr=7 
case 128+56: //Bpp=3 Sr=8 
case 128+57: //Bpp=3 Sr=9 
case 128+58: //Bpp=3 Sr=10 
case 128+59: //Bpp=3 Sr=11 
case 128+60: //Bpp=3 Sr=12 
case 128+61: //Bpp=3 Sr=13 
case 128+62: //Bpp=3 Sr=14 
case 128+63: //Bpp=3 Sr=15 

	return;
	break;
//	XXXXXXXXXXXXXXXXXXXXXX;
case 192+0: //Bpp=0 Sr=0 1BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=1
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		if (!USState32->ScanLines)
		{
			YStride-=(16);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			YStride-=Xpitch;
	}
}
break;

case 192+1: //Bpp=0 Sr=1 1BPP Stretch=2
case 192+2:	//Bpp=0 Sr=2 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=2
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
//************************************************************************************
		if (!MonType)
		{ //Pcolor
			for (Bit=7;Bit>=0;Bit--)
			{
				Pix=(1 & (WidePixel>>Bit) );
				Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
				switch(Sphase)
				{
				case 0:
				case 4:
				case 6:
					Pcolor=0;
					break;
				case 1:
				case 5:
					Pcolor=(Bit &1)+1;
					break;
				case 2:
				//	Pcolor=(!(Bit &1))+1; Use last color
					break;
				case 3:
					Pcolor=3;
					szSurface32[YStride-1]=Afacts32[ColorInvert][3];
					if (!USState32->ScanLines)
						szSurface32[YStride+Xpitch-1]=Afacts32[ColorInvert][3];
					szSurface32[YStride]=Afacts32[ColorInvert][3];
					if (!USState32->ScanLines)
						szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][3];
					break;
				case 7:
					Pcolor=3;
					break;
				} //END Switch

				szSurface32[YStride+=1]=Afacts32[ColorInvert][Pcolor];
				if (!USState32->ScanLines)
					szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][Pcolor];
				szSurface32[YStride+=1]=Afacts32[ColorInvert][Pcolor];
				if (!USState32->ScanLines)
					szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][Pcolor];
				Carry2=Carry1;
				Carry1=Pix;
			}

			for (Bit=15;Bit>=8;Bit--)
			{
				Pix=(1 & (WidePixel>>Bit) );
				Sphase= (Carry2<<2)|(Carry1<<1)|Pix;
				switch(Sphase)
				{
				case 0:
				case 4:
				case 6:
					Pcolor=0;
					break;
				case 1:
				case 5:
					Pcolor=(Bit &1)+1;
					break;
				case 2:
				//	Pcolor=(!(Bit &1))+1; Use last color
					break;
				case 3:
					Pcolor=3;
					szSurface32[YStride-1]=Afacts32[ColorInvert][3];
					if (!USState32->ScanLines)
						szSurface32[YStride+Xpitch-1]=Afacts32[ColorInvert][3];
					szSurface32[YStride]=Afacts32[ColorInvert][3];
					if (!USState32->ScanLines)
						szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][3];
					break;
				case 7:
					Pcolor=3;
					break;
				} //END Switch

				szSurface32[YStride+=1]=Afacts32[ColorInvert][Pcolor];
				if (!USState32->ScanLines)
					szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][Pcolor];
				szSurface32[YStride+=1]=Afacts32[ColorInvert][Pcolor];
				if (!USState32->ScanLines)
					szSurface32[YStride+Xpitch]=Afacts32[ColorInvert][Pcolor];
				Carry2=Carry1;
				Carry1=Pix;
			}

		}
			else
			{
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
			if (!USState32->ScanLines)
			{
				YStride-=(32);
				YStride+=Xpitch;
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
				YStride-=Xpitch;
			}
		}

}
	break;

case 192+3: //Bpp=0 Sr=3 1BPP Stretch=4
case 192+4: //Bpp=0 Sr=4
case 192+5: //Bpp=0 Sr=5
case 192+6: //Bpp=0 Sr=6
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=4
{
	WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];

	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];

	if (!USState32->ScanLines)
	{
		YStride-=(64);
		YStride+=Xpitch;
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		YStride-=Xpitch;
	}
}
break;
case 192+7: //Bpp=0 Sr=7 1BPP Stretch=8 
case 192+8: //Bpp=0 Sr=8
case 192+9: //Bpp=0 Sr=9
case 192+10: //Bpp=0 Sr=10
case 192+11: //Bpp=0 Sr=11
case 192+12: //Bpp=0 Sr=12
case 192+13: //Bpp=0 Sr=13
case 192+14: //Bpp=0 Sr=14
for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //1bbp Stretch=8
{
	WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
	szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];

	if (!USState32->ScanLines)
	{
		YStride-=(128);
		YStride+=Xpitch;
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>7))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>5))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>3))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>1))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>15))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>13))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>11))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>9))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 1 & (WidePixel>>8))];
		YStride-=Xpitch;
	}
}
break;

case 192+15: //Bpp=0 Sr=15 1BPP Stretch=16
case 192+16: //BPP=1 Sr=0  2BPP Stretch=1
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=1
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		if (!USState32->ScanLines)
		{
			YStride-=(8);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=Xpitch;
		}
	}
break;

case 192+17: //Bpp=1 Sr=1  2BPP Stretch=2
case 192+18: //Bpp=1 Sr=2
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=2
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!USState32->ScanLines)
		{
			YStride-=(16);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=Xpitch;
		}
	}
break;

case 192+19: //Bpp=1 Sr=3  2BPP Stretch=4
case 192+20: //Bpp=1 Sr=4
case 192+21: //Bpp=1 Sr=5
case 192+22: //Bpp=1 Sr=6
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=4
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!USState32->ScanLines)
		{
			YStride-=(32);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=Xpitch;
		}
	}
break;

case 192+23: //Bpp=1 Sr=7  2BPP Stretch=8
case 192+24: //Bpp=1 Sr=8
case 192+25: //Bpp=1 Sr=9 
case 192+26: //Bpp=1 Sr=10 
case 192+27: //Bpp=1 Sr=11 
case 192+28: //Bpp=1 Sr=12 
case 192+29: //Bpp=1 Sr=13 
case 192+30: //Bpp=1 Sr=14
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=8
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!USState32->ScanLines)
		{
			YStride-=(64);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=Xpitch;
		}
	}
break;
	
case 192+31: //Bpp=1 Sr=15 2BPP Stretch=16 
	for (HorzBeam=0;HorzBeam<BytesperRow;HorzBeam+=2) //2bbp Stretch=16
	{
		WidePixel=WideBuffer[(VidMask & ( Start+(unsigned char)(Hoffset+HorzBeam) ))>>1];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
		szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];

		if (!USState32->ScanLines)
		{
			YStride-=(128);
			YStride+=Xpitch;
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>6))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>4))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>2))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & WidePixel)];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>14))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>12))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>10))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			szSurface32[YStride+=1]=Pallete32Bit[PalleteIndex+( 3 & (WidePixel>>8))];
			YStride-=Xpitch;
		}
	}
break;

case 192+32: //Bpp=2 Sr=0 4BPP Stretch=1 Unsupport with Compat set
case 192+33: //Bpp=2 Sr=1 4BPP Stretch=2 
case 192+34: //Bpp=2 Sr=2
case 192+35: //Bpp=2 Sr=3 4BPP Stretch=4
case 192+36: //Bpp=2 Sr=4 
case 192+37: //Bpp=2 Sr=5 
case 192+38: //Bpp=2 Sr=6 
case 192+39: //Bpp=2 Sr=7 4BPP Stretch=8
case 192+40: //Bpp=2 Sr=8 
case 192+41: //Bpp=2 Sr=9 
case 192+42: //Bpp=2 Sr=10 
case 192+43: //Bpp=2 Sr=11 
case 192+44: //Bpp=2 Sr=12 
case 192+45: //Bpp=2 Sr=13 
case 192+46: //Bpp=2 Sr=14 
case 192+47: //Bpp=2 Sr=15 4BPP Stretch=16
case 192+48: //Bpp=3 Sr=0  Unsupported 
case 192+49: //Bpp=3 Sr=1 
case 192+50: //Bpp=3 Sr=2 
case 192+51: //Bpp=3 Sr=3 
case 192+52: //Bpp=3 Sr=4 
case 192+53: //Bpp=3 Sr=5 
case 192+54: //Bpp=3 Sr=6 
case 192+55: //Bpp=3 Sr=7 
case 192+56: //Bpp=3 Sr=8 
case 192+57: //Bpp=3 Sr=9 
case 192+58: //Bpp=3 Sr=10 
case 192+59: //Bpp=3 Sr=11 
case 192+60: //Bpp=3 Sr=12 
case 192+61: //Bpp=3 Sr=13 
case 192+62: //Bpp=3 Sr=14 
case 192+63: //Bpp=3 Sr=15 
	return;
	break;

			} //END SWITCH
	return;
}
// END of 32 Bit render loop *****************************************************************************************

void DrawTopBoarder8(SystemState *DTState)
{
	unsigned short x;
	if (BoarderChange==0)
		return;

	for (x=0;x<DTState->WindowSize.x;x++)
	{
		DTState->PTRsurface8[x +((DTState->LineCounter*2)*DTState->SurfacePitch)]=BoarderColor8|128;
		if (!DTState->ScanLines)
			DTState->PTRsurface8[x +((DTState->LineCounter*2+1)*DTState->SurfacePitch)]=BoarderColor8|128;
	}
	return;
}

void DrawTopBoarder16(SystemState *DTState)
{
	unsigned short x;
	if (BoarderChange==0)
		return;

	for (x=0;x<DTState->WindowSize.x;x++)
	{
		DTState->PTRsurface16[x +((DTState->LineCounter*2)*DTState->SurfacePitch)]=BoarderColor16;
		if (!DTState->ScanLines)
			DTState->PTRsurface16[x +((DTState->LineCounter*2+1)*DTState->SurfacePitch)]=BoarderColor16;
	}
	return;
}

void DrawTopBoarder24(SystemState *DTState)
{

	return;
}

void DrawTopBoarder32(SystemState *DTState)
{

	unsigned short x;
	if (BoarderChange==0)
		return;

	for (x=0;x<DTState->WindowSize.x;x++)
	{
		DTState->PTRsurface32[x +((DTState->LineCounter*2)*DTState->SurfacePitch)]=BoarderColor32;
		if (!DTState->ScanLines)
			DTState->PTRsurface32[x +((DTState->LineCounter*2+1)*DTState->SurfacePitch)]=BoarderColor32;
	}
	return;
}

void DrawBottomBoarder8(SystemState *DTState)
{
	if (BoarderChange==0)
		return;	
	unsigned short x;
	for (x=0;x<DTState->WindowSize.x;x++)
	{
		DTState->PTRsurface8[x + (2*(DTState->LineCounter+LinesperScreen+VertCenter) *DTState->SurfacePitch) ]=BoarderColor8|128;
		if (!DTState->ScanLines)
			DTState->PTRsurface8[x + DTState->SurfacePitch+(2*(DTState->LineCounter+LinesperScreen+VertCenter) *DTState->SurfacePitch) ]=BoarderColor8|128;
	}
	return;
}

void DrawBottomBoarder16(SystemState *DTState)
{
	if (BoarderChange==0)
		return;	
	unsigned short x;
	for (x=0;x<DTState->WindowSize.x;x++)
	{
		DTState->PTRsurface16[x + (2*(DTState->LineCounter+LinesperScreen+VertCenter) *DTState->SurfacePitch) ]=BoarderColor16;
		if (!DTState->ScanLines)
			DTState->PTRsurface16[x + DTState->SurfacePitch+(2*(DTState->LineCounter+LinesperScreen+VertCenter) *DTState->SurfacePitch) ]=BoarderColor16;
	}
	return;
}

void DrawBottomBoarder24(SystemState *DTState)
{

	return;
}


void DrawBottomBoarder32(SystemState *DTState)
{
	if (BoarderChange==0)
		return;	
	unsigned short x;

	for (x=0;x<DTState->WindowSize.x;x++)
	{
		DTState->PTRsurface32[x + (2*(DTState->LineCounter+LinesperScreen+VertCenter) *DTState->SurfacePitch) ]=BoarderColor32;
		if (!DTState->ScanLines)
			DTState->PTRsurface32[x + DTState->SurfacePitch+(2*(DTState->LineCounter+LinesperScreen+VertCenter) *DTState->SurfacePitch) ]=BoarderColor32;
	}
	return;
}

void SetBlinkState(unsigned char Tmp)
{
	BlinkState=Tmp;
	return;
}

// These grab the Video info for all COCO 2 modes
void SetGimeVdgOffset (unsigned char Offset)
{
	if ( CC2Offset != Offset)
	{
		CC2Offset=Offset;
		SetupDisplay();
	}
	return;
}

void SetGimeVdgMode (unsigned char VdgMode) //3 bits from SAM Registers
{
	if (CC2VDGMode != VdgMode)
	{
		CC2VDGMode=VdgMode;
		SetupDisplay();
		BoarderChange=3;
	}
	return;
}

void SetGimeVdgMode2 (unsigned char Vdgmode2) //5 bits from PIA Register
{
	if (CC2VDGPiaMode != Vdgmode2)
	{
		CC2VDGPiaMode=Vdgmode2;
		SetupDisplay();
		BoarderChange=3;
	}
	return;
}

//These grab the Video info for all COCO 3 modes

void SetVerticalOffsetRegister(unsigned short Register)
{
	if (VerticalOffsetRegister != Register)
	{
		VerticalOffsetRegister=Register;

		SetupDisplay();
	}
	return;
}

void SetCompatMode( unsigned char Register)
{
	if (CompatMode != Register)
	{
		CompatMode=Register;
		SetupDisplay();
		BoarderChange=3;
	}
	return;
}

void SetGimePallet(unsigned char pallete,unsigned char color)
{
	// Convert the 6bit rgbrgb value to rrrrrggggggbbbbb for the Real video hardware.
	//	unsigned char r,g,b;
	Pallete[pallete]=((color &63));
	Pallete8Bit[pallete]= PalleteLookup8[MonType][color & 63]; 
	Pallete16Bit[pallete]=PalleteLookup16[MonType][color & 63];
	Pallete32Bit[pallete]=PalleteLookup32[MonType][color & 63];
	return;
}

void SetGimeVmode(unsigned char vmode)
{
	if (CC3Vmode != vmode)
	{
		CC3Vmode=vmode;
		SetupDisplay();
		BoarderChange=3;

	}
	return;
}

void SetGimeVres(unsigned char vres)
{
	if (CC3Vres != vres)
	{
		CC3Vres=vres;
		SetupDisplay();
		BoarderChange=3;
	}
	return;
}

void SetGimeHorzOffset(unsigned char data)
{
	if (HorzOffsetReg != data)
	{
		Hoffset=(data<<1);
		HorzOffsetReg=data;
		SetupDisplay();
	}
	return;
}
void SetGimeBoarderColor(unsigned char data)
{

	if (CC3BoarderColor != (data & 63) )
	{
		CC3BoarderColor= data & 63;
		SetupDisplay();
		BoarderChange=3;
	}
	return;
}

void SetBoarderChange (unsigned char data)
{
	if (BoarderChange >0)
		BoarderChange--;
	
	return;
}

void InvalidateBoarder(void)
{
	BoarderChange=5;
	return;
}


void SetupDisplay(void)
{

	static unsigned char CC2Bpp[8]={1,0,1,0,1,0,1,0};
	static unsigned char CC2LinesperRow[8]={12,3,3,2,2,1,1,1};
	static unsigned char CC3LinesperRow[8]={1,1,2,8,9,10,11,200};
	static unsigned char CC2BytesperRow[8]={16,16,32,16,32,16,32,32};
	static unsigned char CC3BytesperRow[8]={16,20,32,40,64,80,128,160};
	static unsigned char CC3BytesperTextRow[8]={32,40,32,40,64,80,64,80};
	static unsigned char CC2PaletteSet[4]={8,0,10,4};
	static unsigned char CCPixelsperByte[4]={8,4,2,2};
	static unsigned char ColorSet=0,Temp1; 
	ExtendedText=1;
	switch (CompatMode)
	{
	case 0:		//Color Computer 3 Mode
		NewStartofVidram=VerticalOffsetRegister*8; 
		GraphicsMode=(CC3Vmode & 128)>>7;
		VresIndex=(CC3Vres & 96) >> 5;
		CC3LinesperRow[7]=LinesperScreen;	// For 1 pixel high modes
		Bpp=CC3Vres & 3;
		LinesperRow=CC3LinesperRow[CC3Vmode & 7];
		BytesperRow=CC3BytesperRow[(CC3Vres & 28)>> 2];
		PalleteIndex=0;
		if (!GraphicsMode)
		{
			if (CC3Vres & 1)
				ExtendedText=2;
			Bpp=0;
			BytesperRow=CC3BytesperTextRow[(CC3Vres & 28)>> 2];
		}
		break;

	case 1:					//Color Computer 2 Mode
		CC3BoarderColor=0;	//Black for text modes
		BoarderChange=3;
		NewStartofVidram= (512*CC2Offset)+(VerticalOffsetRegister & 0xE0FF)*8; 
		GraphicsMode=( CC2VDGPiaMode & 16 )>>4; //PIA Set on graphics clear on text
		VresIndex=0;
		LinesperRow= CC2LinesperRow[CC2VDGMode];

		if (GraphicsMode)
		{
			CC3BoarderColor=63;
			BoarderChange=3;
			Bpp=CC2Bpp[ (CC2VDGPiaMode & 15) >>1 ];
			BytesperRow=CC2BytesperRow[ (CC2VDGPiaMode & 15) >>1 ];
			Temp1= (CC2VDGPiaMode &1) << 1 | (Bpp & 1);
			PalleteIndex=CC2PaletteSet[Temp1];
		}
		else
		{	//Setup for 32x16 text Mode
			Bpp=0;
			BytesperRow=32;
			InvertAll= (CC2VDGPiaMode & 4)>>2;
			LowerCase= (CC2VDGPiaMode & 2)>>1;
			ColorSet = (CC2VDGPiaMode & 1);
			Temp1= ( (ColorSet<<1) | InvertAll);
			switch (Temp1)
			{
			case 0:
				TextFGPallete=12;
				TextBGPallete=13;
				break;
			case 1:
				TextFGPallete=13;
				TextBGPallete=12;
				break;
			case 2:
				TextFGPallete=14;
				TextBGPallete=15;
				break;
			case 3:
				TextFGPallete=15;
				TextBGPallete=14;
				break;
			}
		}
		break;
}
	ColorInvert= (CC3Vmode & 32)>>5;
	LinesperScreen=Lpf[VresIndex];
	SetLinesperScreen(VresIndex);
	VertCenter=VcenterTable[VresIndex]-4; //4 unrendered top lines
	PixelsperLine= BytesperRow*CCPixelsperByte[Bpp];
	PixelsperByte=CCPixelsperByte[Bpp];

	if (PixelsperLine % 40)
	{
		Stretch = (512/PixelsperLine)-1;
		HorzCenter=64;
	}
	else
	{
		Stretch = (640/PixelsperLine)-1;
		HorzCenter=0;
	}
	VPitch=BytesperRow;
	if (HorzOffsetReg & 128)
		VPitch=256;
	BoarderColor8=((CC3BoarderColor & 63) |128);
	BoarderColor16=PalleteLookup16[MonType][CC3BoarderColor & 63];
	BoarderColor32=PalleteLookup32[MonType][CC3BoarderColor & 63];
	NewStartofVidram =(NewStartofVidram & VidMask)+DistoOffset; //DistoOffset for 2M configuration
	MasterMode= (GraphicsMode <<7) | (CompatMode<<6)  | ((Bpp & 3)<<4) | (Stretch & 15);
	return;
}


void GimeInit(void)
{
	//Nothing but good to have.
	return;
}



void GimeReset(void)
{
	CC3Vmode=0;
	CC3Vres=0;
	StartofVidram=0;
	NewStartofVidram=0;
	GraphicsMode=0;
	LowerCase=0;
	InvertAll=0;
	ExtendedText=1;
	HorzOffsetReg=0;
	TagY=0;
	DistoOffset=0;
	MakeRGBPalette ();
	MakeCMPpalette();
	BoarderChange=3;
	CC2Offset=0;
	Hoffset=0;
	VerticalOffsetRegister=0;
	MiscReset();
	return;
}

void SetVidMask(unsigned int data)
{
	VidMask=data;
	return;
}

void SetVideoBank(unsigned char data)
{
	DistoOffset= data * (512*1024);
	SetupDisplay();
	return;
}


void MakeRGBPalette (void)
{
	unsigned char Index=0;
	unsigned char r,g,b;
	for (Index=0;Index<64;Index++)
	{

		PalleteLookup8[1][Index]= Index |128;

		r= ColorTable16Bit [(Index & 32) >> 4 | (Index & 4) >> 2];
		g= ColorTable16Bit [(Index & 16) >> 3 | (Index & 2) >> 1];
		b= ColorTable16Bit [(Index & 8 ) >> 2 | (Index & 1) ];
		PalleteLookup16[1][Index]= (r<<11) | (g << 6) | b;
	//32BIT
		r= ColorTable32Bit [(Index & 32) >> 4 | (Index & 4) >> 2];	
		g= ColorTable32Bit [(Index & 16) >> 3 | (Index & 2) >> 1];	
		b= ColorTable32Bit [(Index & 8 ) >> 2 | (Index & 1) ];		
		PalleteLookup32[1][Index]= (r* 65536) + (g* 256) + b;
		
		
		
	}
	return;
}


void MakeCMPpalette(void)	
{
	double saturation, brightness, contrast;
	int offset;
	double w;
	double r,g,b;
	
	int PaletteType = GetPaletteType();

	unsigned char rr,gg,bb;
	unsigned char Index=0;

	
	int red[] = { 
		0,14,12,21,51,86,108,118,
		113,92,61,21,1,5,12,13,
		50,29,49,86,119,158,179,192,
		186,165,133,94,23,16,23,25,
		116,74,102,142,179,219,243,252,
		251,230,198,155,81,61,52,57,
		253,137,161,189,215,240,253,253,
		251,237,214,183,134,121,116,255
	};
	int green[] = { 
		0,78,69,53,33,4,1,1,
		12,24,31,35,37,51,67,77,
		50,149,141,123,103,77,55,39,
		35,43,53,63,100,119,137,148,
		116,212,204,186,164,137,114,97,
		88,89,96,109,156,179,199,211,
		253,230,221,207,192,174,158,148,
		143,144,150,162,196,212,225,255
	};
	int blue[] = {
		0,20,18,14,10,10,12,19,
		76,135,178,196,148,97,29,20,
		50,38,36,32,28,25,24,78,
		143,207,248,249,228,174,99,46,
		116,58,52,48,44,41,68,132,
		202,250,250,250,251,243,163,99,
		254,104,83,77,82,105,142,188,
		237,251,251,251,252,240,183,255
	};

	float gamma = 1.4;
	if (PaletteType == 1) { OutputDebugString("Loading new CMP palette.\n"); }
	else { OutputDebugString("Loading old CMP palette.\n"); }
	for (Index = 0; Index <= 63; Index++)
	{
		if (PaletteType == 1) 
		{
			if (Index > 39) { gamma = 1.1; }
			if (Index > 55) { gamma = 1; }

			//int tmp = 0;

			r = red[Index] * gamma; if (r > 255) { r = 255; }
			g = green[Index] * gamma; if (g > 255) { g = 255; }
			b = blue[Index] * gamma; if (b > 255) { b = 255; }
		}
		else {  //Old palette //Stolen from M.E.S.S.
					switch(Index)
					{
						case 0:
							r = g = b = 0;
							break;
						case 16:
							r = g = b = 47;
							break;
			
						case 32:
							r = g = b = 120;
							break;
			
						case 48:
						case 63:
							r = g = b = 255;
							break;
			
						default:
							w = .4195456981879*1.01;
							contrast = 70;
							saturation = 92;
							brightness = -20;
							brightness += ((Index / 16) + 1) * contrast;
							offset = (Index % 16) - 1 + (Index / 16)*15;
							r = cos(w*(offset +  9.2)) * saturation + brightness;
							g = cos(w*(offset + 14.2)) * saturation + brightness;
							b = cos(w*(offset + 19.2)) * saturation + brightness;
			
							if (r < 0)
								r = 0;
							else if (r > 255)
								r = 255;
			
							if (g < 0)
								g = 0;
							else if (g > 255)
								g = 255;
			
							if (b < 0)
								b = 0;
							else if (b > 255)
								b = 255;
							break;

				}
		}
		rr= (unsigned char)r;
		gg= (unsigned char)g;
		bb= (unsigned char)b;
		PalleteLookup32[0][Index]= (rr<<16) | (gg<<8) | bb;
		rr=rr>>3;
		gg=gg>>3;
		bb=bb>>3;
		PalleteLookup16[0][Index]= (rr<<11) | (gg<<6) | bb;
		rr=rr>>3;
		gg=gg>>3;
		bb=bb>>3;
		PalleteLookup8[0][Index]=0x80 | ((rr & 2)<<4) | ((gg & 2)<<3) | ((bb & 2)<<2) | ((rr & 1)<<2) | ((gg & 1)<<1) | (bb & 1);
	}
}

unsigned char SetMonitorType(unsigned char Type)
{
	unsigned char PalNum=0;
	SetGimeBoarderColor(0);
	if (Type != QUERY)
	{
		MonType=Type & 1;
		for (PalNum=0;PalNum<16;PalNum++)
		{
			Pallete16Bit[PalNum]=PalleteLookup16[MonType][Pallete[PalNum]];
			Pallete32Bit[PalNum]=PalleteLookup32[MonType][Pallete[PalNum]];
			Pallete8Bit[PalNum]= PalleteLookup8[MonType][Pallete[PalNum]];
		}
//		CurrentConfig.MonitorType=MonType;
	}
	SetGimeBoarderColor(18);
	return(MonType);
}
void SetPaletteType() {
	 
	SetGimeBoarderColor(0);
	MakeCMPpalette();
	SetGimeBoarderColor(18);
}

unsigned char SetScanLines(unsigned char Lines)
{
	extern SystemState EmuState;
	if (Lines!=QUERY)
	{
		EmuState.ScanLines=Lines;
		Cls(0,&EmuState);
		BoarderChange=3;
	}
	return(0);
}
int GetBytesPerRow() {
	return BytesperRow;
}

unsigned int GetStartOfVidram() {
	return StartofVidram;
}
int GetGraphicsMode() {
	return(GraphicsMode);
}

/*
unsigned char SetArtifacts(unsigned char Tmp)
{
	if (Tmp!=QUERY)
	Artifacts=Tmp;
	return(Artifacts);
}
*/
/*
unsigned char SetColorInvert(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		ColorInvert=Tmp;
	return(ColorInvert);
}
*/


