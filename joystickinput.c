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

#include "joystickinput.h"

JoyStick Left;
JoyStick Right;

static unsigned short StickValue = 0;
static unsigned short LeftStickX = 32;
static unsigned short LeftStickY = 32;
static unsigned short RightStickX = 32;
static unsigned short RightStickY = 32;
static unsigned char LeftButton1Status = 0;
static unsigned char RightButton1Status = 0;
static unsigned char LeftButton2Status = 0;
static unsigned char RightButton2Status = 0;
static unsigned char LeftStickNumber = 0;
static unsigned char RightStickNumber = 0;

static LPDIRECTINPUTDEVICE8 Joysticks[MAXSTICKS];
char StickName[MAXSTICKS][STRLEN];
static unsigned char JoyStickIndex=0;
static LPDIRECTINPUT8 di;
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* , VOID* );
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* , VOID* );
static unsigned char CurrentStick;

int EnumerateJoysticks(void)
{
	HRESULT hr;
	JoyStickIndex=0;
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL),
                    DIRECTINPUT_VERSION,IID_IDirectInput8,(VOID**)&di,NULL)))
		return(0);

	if (FAILED(hr = di->EnumDevices(DI8DEVCLASS_GAMECTRL,
                    enumCallback,NULL,DIEDFL_ATTACHEDONLY)))
		return(0);
	return(JoyStickIndex);
}

BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
	HRESULT hr;
	hr = di->CreateDevice(instance->guidInstance, &Joysticks[JoyStickIndex],NULL);
	strncpy(StickName[JoyStickIndex],instance->tszProductName,STRLEN);
	JoyStickIndex++;
	return(JoyStickIndex<MAXSTICKS);
}


bool InitJoyStick (unsigned char StickNumber)
{
//	DIDEVCAPS capabilities;
	HRESULT hr;
	CurrentStick=StickNumber;
	if (Joysticks[StickNumber]==NULL)
		return(0);

	if (FAILED(hr= Joysticks[StickNumber]->SetDataFormat(&c_dfDIJoystick2)))
		return(0);

//	if (FAILED(hr= Joysticks[StickNumber]->SetCooperativeLevel(NULL, DISCL_EXCLUSIVE )))
//		return(0);

	//Fails for some reason Investigate this
//	if (FAILED(hr= Joysticks[StickNumber]->GetCapabilities(&capabilities)))
//		return(0);

	if (FAILED(hr= Joysticks[StickNumber]->EnumObjects(enumAxesCallback,NULL,DIDFT_AXIS)))
		return(0);
	return(1); //return true on success
}


BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context)
{
	HWND hDlg= (HWND)context;
	DIPROPRANGE propRange;
	propRange.diph.dwSize = sizeof(DIPROPRANGE);
	propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	propRange.diph.dwHow = DIPH_BYID;
	propRange.diph.dwObj = instance->dwType;
	propRange.lMin=0;
	propRange.lMax=0xFFFF;

	if (FAILED(Joysticks[CurrentStick]->SetProperty(DIPROP_RANGE, &propRange.diph)))
		return(DIENUM_STOP);

	return(DIENUM_CONTINUE);
}

/*****************************************************************************/
HRESULT JoyStickPoll(DIJOYSTATE2 *js,unsigned char StickNumber)
{
	HRESULT hr;
	if (Joysticks[StickNumber] ==NULL)
		return (S_OK);

	hr=Joysticks[StickNumber]->Poll();
	if (FAILED(hr))
	{
		hr=Joysticks[StickNumber]->Acquire();
		while (hr == DIERR_INPUTLOST)
			hr = Joysticks[StickNumber]->Acquire();

		if (hr == DIERR_INVALIDPARAM)  //|| (hr == DIERR_NOTINITALIZED
			return(E_FAIL);

		if (hr == DIERR_OTHERAPPHASPRIO)
			return(S_OK);
	}

	if (FAILED(hr= Joysticks[StickNumber]->GetDeviceState(sizeof(DIJOYSTATE2), js)))
		return(hr);

    return(S_OK);

}

/*****************************************************************************/
// Called by keyboard.c to add joystick bits to scancode
unsigned char
vccJoystickGetScan(unsigned char ret_val)
{

    //	MuxSelect=GetMuxState();	//Collect CA2 and CB2 from the PIA (1of4 Multiplexer)
	StickValue = get_pot_value(GetMuxState());
	if (StickValue != 0)		//OS9 joyin routine needs this (koronis rift works now)
	{
		if (StickValue >= DACState())		// Set bit of stick >= DAC output $FF20 Bits 7-2
		{
			ret_val |= 0x80;
		}
	}

	if (LeftButton1Status == 1)
	{
		//Left Joystick Button 1 Down?
		ret_val = ret_val & 0xFD;
	}

	if (RightButton1Status == 1)
	{
		//Right Joystick Button 1 Down?
		ret_val = ret_val & 0xFE;
	}

	if (LeftButton2Status == 1)
	{
		//Left Joystick Button 2 Down?
		ret_val = ret_val & 0xF7;
	}

	if (RightButton2Status == 1)
	{
		//Right Joystick Button 2 Down?
		ret_val = ret_val & 0xFB;
	}
    return ret_val;
}

/*****************************************************************************/
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

/*****************************************************************************/
void
SetStickNumbers(unsigned char Temp1,unsigned char Temp2)
{
	LeftStickNumber=Temp1;
	RightStickNumber=Temp2;
	return;
}

/*****************************************************************************/
unsigned short
get_pot_value(unsigned char pot)
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

	return (0);
}

/*****************************************************************************/
unsigned char
SetMouseStatus(unsigned char ScanCode,unsigned char Phase)
{
	unsigned char ReturnValue=ScanCode;

	// Mask scan code high bit to accept keys from extended keyboard arrow
	// keypad. A more elegant solution would be to use the virtual key code
	// for joystick mappings but that would require changes to config.c as well.
    unsigned char TempCode = ScanCode & 0x7F;

	switch (Phase)
	{
	case 0:
		if (Left.UseMouse==0)
		{
			if (TempCode==Left.Left)
			{
				LeftStickX=32;
				ReturnValue=0;
			}
			if (TempCode==Left.Right)
			{
				LeftStickX=32;
				ReturnValue=0;
			}
			if (TempCode==Left.Up)
			{
				LeftStickY=32;
				ReturnValue=0;
			}
			if (TempCode==Left.Down)
			{
				LeftStickY=32;
				ReturnValue=0;
			}
			if (TempCode==Left.Fire1)
			{
				LeftButton1Status=0;
				ReturnValue=0;
			}
			if (TempCode==Left.Fire2)
			{
				LeftButton2Status=0;
				ReturnValue=0;
			}
		}

		if (Right.UseMouse==0)
		{
			if (TempCode==Right.Left)
			{
				RightStickX=32;
				ReturnValue=0;
			}
			if (TempCode==Right.Right)
			{
				RightStickX=32;
				ReturnValue=0;
			}
			if (TempCode==Right.Up)
			{
				RightStickY=32;
				ReturnValue=0;
			}
			if (TempCode==Right.Down)
			{
				RightStickY=32;
				ReturnValue=0;
			}
			if (TempCode==Right.Fire1)
			{
				RightButton1Status=0;
				ReturnValue=0;
			}
			if (TempCode==Right.Fire2)
			{
				RightButton2Status=0;
				ReturnValue=0;
			}
		}
	break;

	case 1:
		if (Left.UseMouse==0)
		{
			if (TempCode==Left.Left)
			{
				LeftStickX=0;
				ReturnValue=0;
			}
			if (TempCode==Left.Right)
			{
				LeftStickX=63;
				ReturnValue=0;
			}
			if (TempCode==Left.Up)
			{
				LeftStickY=0;
				ReturnValue=0;
			}
			if (TempCode==Left.Down)
			{
				LeftStickY=63;
				ReturnValue=0;
			}
			if (TempCode==Left.Fire1)
			{
				LeftButton1Status=1;
				ReturnValue=0;
			}
			if (TempCode==Left.Fire2)
			{
				LeftButton2Status=1;
				ReturnValue=0;
			}
		}

		if (Right.UseMouse==0)
		{
			if (TempCode==Right.Left)
			{
				ReturnValue=0;
				RightStickX=0;
			}
			if (TempCode==Right.Right)
			{
				RightStickX=63;
				ReturnValue=0;
			}
			if (TempCode==Right.Up)
			{
				RightStickY=0;
				ReturnValue=0;
			}
			if (TempCode==Right.Down)
			{
				RightStickY=63;
				ReturnValue=0;
			}
			if (TempCode==Right.Fire1)
			{
				RightButton1Status=1;
				ReturnValue=0;
			}
			if (TempCode==Right.Fire2)
			{
				RightButton2Status=1;
				ReturnValue=0;
			}
		}
	break;
	}

	return(ReturnValue);
}

/****************************************************************************/

//Side=0 Left Button Side=1 Right Button State 1=Down

void
SetButtonStatus(unsigned char Side,unsigned char State)
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
}

