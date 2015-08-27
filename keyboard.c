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

#define DIRECTINPUT_VERSION 0x0800

#include <windows.h>
#include <stdio.h>
#include <dinput.h>
#include "defines.h"
#include "keyboard.h"
#include "mc6821.h"
#include "tcc1014registers.h"
#include "Vcc.h"
#include "joystickinput.h"

void UpdateScanTable(void);
char SetMouseStatus(char ,unsigned char );
static unsigned char KeyboardInteruptEnabled=0;
static unsigned char ScanTable[256]="";
static unsigned char RolloverTable[8]={0,0,0,0,0,0,0,0};
extern JoyStick Left;
extern JoyStick Right;
static unsigned short LeftStickX=32,LeftStickY=32,RightStickX=32,RightStickY=32;
static unsigned char LeftButton1Status=0,RightButton1Status=0,LeftButton2Status=0,RightButton2Status=0;
static unsigned char LeftStickNumber=0,RightStickNumber=0;
extern  KeyBoardTable Table [256];


void KeyboardEvent( unsigned char key, unsigned char ScanCode,unsigned int Status)

{	//status 0=up 2=down 1=unused
	unsigned char Index=0;
	switch (Status)
	{
	case 2:		//Key Down
		if (ScanCode==54) //Left and right shift generate different scan codes
			ScanCode=42;
		if ( (Left.UseMouse==0) | (Right.UseMouse==0) )
			ScanCode=SetMouseStatus(ScanCode,1);
		ScanTable[ScanCode]=1;
		UpdateScanTable();
		if (KeyboardInteruptEnabled)
			GimeAssertKeyboardInterupt();

	break;

	case 0:		//Key Up
		if (ScanCode==54)
			ScanCode=42;
		if ( (Left.UseMouse==0) | (Right.UseMouse==0) )
			ScanCode=SetMouseStatus(ScanCode,0);
		ScanTable[ScanCode]=0;
		if (ScanCode==42) //Clean out rollover table on shift release
		{
			for (Index=0;Index<100;Index++)
				ScanTable[Index]=0;
		}
		UpdateScanTable();


		break;
	}
	return;
}

unsigned char kb_scan(unsigned char Col)

{
	unsigned char temp,x,mask,ret_val=0;
	static unsigned char IrqFlag=0;
	static unsigned short StickValue=0;
	temp=~Col; //Get colums
	mask=1;
	ret_val=0;
	for (x=0;x<=7;x++)
	{
		if (temp & mask) //Found an active column scan
			ret_val|= RolloverTable[x];
		mask=(mask<<1);
	}
	ret_val=127-ret_val;
	
//	MuxSelect=GetMuxState();	//Collect CA2 and CB2 from the PIA (1of4 Multiplexer)
	StickValue=get_pot_value (GetMuxState());
	if (StickValue!=0)		//OS9 joyin routine needs this (koronis rift works now)
		if (StickValue >= DACState())		// Set bit of stick >= DAC output $FF20 Bits 7-2
			ret_val|= 0x80;

	if (LeftButton1Status==1)
		ret_val=ret_val & 0xFD;	//Left Joystick Button 1 Down?

	if (RightButton1Status==1)
		ret_val=ret_val & 0xFE;	//Right Joystick Button 1 Down?

	if (LeftButton2Status==1)
		ret_val=ret_val & 0xF7;	//Left Joystick Button 2 Down?

	if (RightButton2Status==1)
		ret_val=ret_val & 0xFB;	//Right Joystick Button 2 Down?

	if ((ret_val & 0x7F)!=0x7F) 
	{
		if ( (IrqFlag==0) & (KeyboardInteruptEnabled==1) )
		{
			GimeAssertKeyboardInterupt();
			IrqFlag=1;
		}
	}
	else
		IrqFlag=0;

	return(ret_val);
}

void SetKeyboardInteruptState(unsigned char State)
{
	KeyboardInteruptEnabled= !!State;
	return;
}

void UpdateScanTable(void)
{
	static unsigned char Index;
	unsigned char LockOut=0;
	for (Index=0;Index<=7;Index++)
		RolloverTable[Index]=0;

	for (Index=0;Index<100;Index++)
	{
		if (LockOut != Table[Index].ScanCode1)
		{
			if ( (Table[Index].ScanCode1!=0) & (Table[Index].ScanCode2==0) )	//Single input key 
			{
				if (ScanTable[ Table[Index].ScanCode1] ==1)
				{
					RolloverTable[Table[Index].Col1]|=Table[Index].Row1;
					RolloverTable[Table[Index].Col2]|=Table[Index].Row2;
				}
			}
			
			if ( (Table[Index].ScanCode1!=0) & (Table[Index].ScanCode2!=0) ) //Double Input Key
			{
				if ( (ScanTable [Table[Index].ScanCode1] ==1) & (ScanTable[Table[Index].ScanCode2]==1))
				{
					LockOut = Table[Index].ScanCode1;
					RolloverTable[Table[Index].Col1]|=Table[Index].Row1;
					RolloverTable[Table[Index].Col2]|=Table[Index].Row2;
					break;
				}
			}
		}
	}
	return;
}


void joystick(unsigned short x,unsigned short y)
{

	if (x>63)
		x=63;
	if (y>63)
		y=63;
	if (Left.UseMouse==1)
	{
		LeftStickX=x;
		LeftStickY=y;
	}
	if (Right.UseMouse==1)
	{
		RightStickX=x;
		RightStickY=y;
	}

	return;
}

void SetStickNumbers(unsigned char Temp1,unsigned char Temp2)
{
	LeftStickNumber=Temp1;
	RightStickNumber=Temp2;
	return;
}

unsigned short get_pot_value(unsigned char pot)
{
	DIJOYSTATE2 Stick1;
	if (Left.UseMouse==3)
	{
		JoyStickPoll(&Stick1,LeftStickNumber);
		LeftStickX =(unsigned short)Stick1.lX>>10;
		LeftStickY= (unsigned short)Stick1.lY>>10;
		LeftButton1Status= Stick1.rgbButtons[0]>>7;
		LeftButton2Status= Stick1.rgbButtons[1]>>7;
	}

	if (Right.UseMouse ==3)
	{
		JoyStickPoll(&Stick1,RightStickNumber);
		RightStickX= (unsigned short)Stick1.lX>>10;
		RightStickY= (unsigned short)Stick1.lY>>10;
		RightButton1Status= Stick1.rgbButtons[0]>>7;
		RightButton2Status= Stick1.rgbButtons[1]>>7;
	}

switch (pot)
{
	case 0:
		return(RightStickX);
		break;

	case 1:
		return(RightStickY);
		break;

	case 2:
		return(LeftStickX);
		break;

	case 3:
		return(LeftStickY);
		break;
}
return(0);
}

char SetMouseStatus(char ScanCode,unsigned char Phase)
{
	char ReturnValue=ScanCode;
	switch (Phase)
	{
	case 0:
		if (Left.UseMouse==0)
		{
			if (ScanCode==Left.Left)
			{
				LeftStickX=32;
				ReturnValue=0;
			}
			if (ScanCode==Left.Right)
			{
				LeftStickX=32;
				ReturnValue=0;
			}
			if (ScanCode==Left.Up)
			{
				LeftStickY=32;
				ReturnValue=0;
			}
			if (ScanCode==Left.Down)
			{
				LeftStickY=32;
				ReturnValue=0;
			}
			if (ScanCode==Left.Fire1)
			{
				LeftButton1Status=0;
				ReturnValue=0;
			}
			if (ScanCode==Left.Fire2)
			{
				LeftButton2Status=0;
				ReturnValue=0;
			}
		}

		if (Right.UseMouse==0)
		{
			if (ScanCode==Right.Left)
			{
				RightStickX=32;
				ReturnValue=0;
			}
			if (ScanCode==Right.Right)
			{
				RightStickX=32;
				ReturnValue=0;
			}
			if (ScanCode==Right.Up)
			{
				RightStickY=32;
				ReturnValue=0;
			}
			if (ScanCode==Right.Down)
			{
				RightStickY=32;
				ReturnValue=0;
			}
			if (ScanCode==Right.Fire1)
			{
				RightButton1Status=0;
				ReturnValue=0;
			}
			if (ScanCode==Right.Fire2)
			{
				RightButton2Status=0;
				ReturnValue=0;
			}
		}
	break;

	case 1:
		if (Left.UseMouse==0)
		{
			if (ScanCode==Left.Left)
			{
				LeftStickX=0;
				ReturnValue=0;
			}
			if (ScanCode==Left.Right)
			{
				LeftStickX=63;
				ReturnValue=0;
			}
			if (ScanCode==Left.Up)
			{
				LeftStickY=0;
				ReturnValue=0;
			}
			if (ScanCode==Left.Down)
			{
				LeftStickY=63;
				ReturnValue=0;
			}
			if (ScanCode==Left.Fire1)
			{
				LeftButton1Status=1;
				ReturnValue=0;
			}
			if (ScanCode==Left.Fire2)
			{
				LeftButton2Status=1;
				ReturnValue=0;
			}
		}

		if (Right.UseMouse==0)
		{
			if (ScanCode==Right.Left)
			{
				ReturnValue=0;
				RightStickX=0;
			}
			if (ScanCode==Right.Right)
			{
				RightStickX=63;
				ReturnValue=0;
			}
			if (ScanCode==Right.Up)
			{
				RightStickY=0;
				ReturnValue=0;
			}
			if (ScanCode==Right.Down)
			{
				RightStickY=63;
				ReturnValue=0;
			}
			if (ScanCode==Right.Fire1)
			{
				RightButton1Status=1;
				ReturnValue=0;
			}
			if (ScanCode==Right.Fire2)
			{
				RightButton2Status=1;
				ReturnValue=0;
			}
		}
	break;
	}
	return(ReturnValue);
}

void SetButtonStatus(unsigned char Side,unsigned char State) //Side=0 Left Button Side=1 Right Button State 1=Down
{
unsigned char Btemp=0;
	Btemp= (Side<<1) | State;
	if (Left.UseMouse==1)
		switch (Btemp)
		{
		case 0:
			LeftButton1Status=0;
		break;

		case 1:
			LeftButton1Status=1;
		break;

		case 2:
			LeftButton2Status=0;
		break;

		case 3:
			LeftButton2Status=1;
		break;
		}

	if (Right.UseMouse==1)
		switch (Btemp)
		{
		case 0:
			RightButton1Status=0;
		break;

		case 1:
			RightButton1Status=1;
		break;

		case 2:
			RightButton2Status=0;
		break;

		case 3:
			RightButton2Status=1;
		break;
		}
	return;
}



