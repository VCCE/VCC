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
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,IID_IDirectInput8,(VOID**)&di,NULL)))
		return(0);

	if (FAILED(hr = di->EnumDevices(DI8DEVCLASS_GAMECTRL, enumCallback,NULL,DIEDFL_ATTACHEDONLY)))
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
