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

// These were renamed from Left and Right to preserve maintainer sanity
JoyStick LeftJS;
JoyStick RightJS;

/*
/ Stock coco joysticks only support 0-63 due to use of 6 bit DAC compare
/ But highest possible rez for CoCo3 screen is 640x225.
/
/ Originaly Vcc joystick values were 0-63 with midpoint 32  (6 bits)
/ CocoMax cart used a ADC that retuned an 8 bit value.
/ Windows HID joysticks have 16 bit resolution.
/
/ Tandy Hirez joysticks used special code that timed how long it takes 
/ joystick values to respond to voltage changes.
/
/
*/

// Joystick values  (0-63)
static unsigned short LeftStickX = 32;
static unsigned short LeftStickY = 32;
static unsigned short RightStickX = 32;
static unsigned short RightStickY = 32;

static unsigned short StickValue = 0;
static unsigned char LeftStickNumber = 0;
static unsigned char RightStickNumber = 0;

// Button states
static unsigned char LeftButton1Status = 0;
static unsigned char RightButton1Status = 0;
static unsigned char LeftButton2Status = 0;
static unsigned char RightButton2Status = 0;

static LPDIRECTINPUTDEVICE8 Joysticks[MAXSTICKS];
char StickName[MAXSTICKS][STRLEN];
static unsigned char JoyStickIndex=0;
static LPDIRECTINPUT8 di;
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* , VOID* );
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* , VOID* );
static unsigned char CurrentStick;

/*****************************************************************************/
// Locate connected joysticks.  Called by config.c
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

/*****************************************************************************/
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
    HRESULT hr;
    hr = di->CreateDevice(instance->guidInstance, &Joysticks[JoyStickIndex],NULL);
    strncpy(StickName[JoyStickIndex],instance->tszProductName,STRLEN);
    JoyStickIndex++;
    return(JoyStickIndex<MAXSTICKS);
}

/*****************************************************************************/
// Init joystick called by config.c
bool InitJoyStick (unsigned char StickNumber)
{
//  DIDEVCAPS capabilities;
    HRESULT hr;
    CurrentStick=StickNumber;
    if (Joysticks[StickNumber]==NULL)
        return(0);

    if (FAILED(hr= Joysticks[StickNumber]->SetDataFormat(&c_dfDIJoystick2)))
        return(0);

//  if (FAILED(hr= Joysticks[StickNumber]->SetCooperativeLevel(NULL, DISCL_EXCLUSIVE )))
//      return(0);

    //Fails for some reason Investigate this
//  if (FAILED(hr= Joysticks[StickNumber]->GetCapabilities(&capabilities)))
//      return(0);

    if (FAILED(hr= Joysticks[StickNumber]->EnumObjects(enumAxesCallback,NULL,DIDFT_AXIS)))
        return(0);
    return(1); //return true on success
}

/*****************************************************************************/
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
// Called by get_pot_value to read connected joystick
HRESULT 
JoyStickPoll(DIJOYSTATE2 *js,unsigned char StickNumber)
{
    HRESULT hr;
    if (Joysticks[StickNumber] ==NULL)
        return (S_OK);

    hr=Joysticks[StickNumber]->Poll();
    if (FAILED(hr)) {
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
    StickValue = get_pot_value(GetMuxState());
    if (StickValue != 0) {               // OS9 joyin routine needs this (koronis rift works now)
        if (StickValue >= DACState()) {  // Set bit if stick >= DAC output $FF20 Bits 7-2
            ret_val |= 0x80;
        }
    }

    if (LeftButton1Status == 1) {
        //Left Joystick Button 1 Down?
        ret_val = ret_val & 0xFD;
    }

    if (RightButton1Status == 1) {
        //Right Joystick Button 1 Down?
        ret_val = ret_val & 0xFE;
    }

    if (LeftButton2Status == 1) {
        //Left Joystick Button 2 Down?
        ret_val = ret_val & 0xF7;
    }

    if (RightButton2Status == 1) {
        //Right Joystick Button 2 Down?
        ret_val = ret_val & 0xFB;
    }
    return ret_val;
}

/*****************************************************************************/
// Called by vcc.c on mouse moves

void joystick(unsigned int x,unsigned int y)
{

/*
    if (x>63) x=63;
    if (y>63) y=63;

    if (LeftJS.UseMouse==1) {
        LeftStickX=x;
        LeftStickY=y;
    }
    if (RightJS.UseMouse==1) {
        RightStickX=x;
        RightStickY=y;
    }
    return;
}
*/

    unsigned short xlo = LOBYTE(x);
    unsigned short ylo = LOBYTE(y);
    unsigned short xhi = HIBYTE(x);
    unsigned short yhi = HIBYTE(y);

    if (xhi>63) xhi=63;
    if (yhi>63) yhi=63;

    if (LeftJS.UseMouse==1) {
        LeftStickX=xhi;
        LeftStickY=yhi;
    }
    if (RightJS.UseMouse==1) {
        RightStickX=xhi;
        RightStickY=yhi;
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
// Called by vccJoystickGetScan
unsigned short
get_pot_value(unsigned char pot)
{
    DIJOYSTATE2 Stick1;

    if (LeftJS.UseMouse==3) {
        JoyStickPoll(&Stick1,LeftStickNumber);
        LeftStickX =(unsigned short)Stick1.lX>>10;
        LeftStickY= (unsigned short)Stick1.lY>>10;
        LeftButton1Status= Stick1.rgbButtons[0]>>7;
        LeftButton2Status= Stick1.rgbButtons[1]>>7;
    }

    if (RightJS.UseMouse ==3) {
        JoyStickPoll(&Stick1,RightStickNumber);
        RightStickX= (unsigned short)Stick1.lX>>10;
        RightStickY= (unsigned short)Stick1.lY>>10;
        RightButton1Status= Stick1.rgbButtons[0]>>7;
        RightButton2Status= Stick1.rgbButtons[1]>>7;
    }

    switch (pot) {
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
// Called by keyboard.h to handle keyboard joysticks

unsigned char
SetMouseStatus(unsigned char ScanCode,unsigned char Phase)
{
    unsigned char ReturnValue=ScanCode;

    // Mask scan code high bit to accept keys from extended keyboard arrow
    // keypad. A more elegant solution would be to use the virtual key code
    // for joystick mappings but that would require changes to config.c as well.
    unsigned char TempCode = ScanCode & 0x7F;

    switch (Phase) {
    case 0:
        if (LeftJS.UseMouse==0) {
            if (TempCode==LeftJS.Left) {
                LeftStickX=32;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Right) {
                LeftStickX=32;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Up) {
                LeftStickY=32;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Down) {
                LeftStickY=32;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Fire1) {
                LeftButton1Status=0;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Fire2) {
                LeftButton2Status=0;
                ReturnValue=0;
            }
        }

        if (RightJS.UseMouse==0) {
            if (TempCode==RightJS.Left) {
                RightStickX=32;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Right) {
                RightStickX=32;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Up) {
                RightStickY=32;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Down) {
                RightStickY=32;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Fire1) {
                RightButton1Status=0;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Fire2) {
                RightButton2Status=0;
                ReturnValue=0;
            }
        }
        break;

    case 1:
        if (LeftJS.UseMouse==0) {
            if (TempCode==LeftJS.Left) {
                LeftStickX=0;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Right) {
                LeftStickX=63;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Up) {
                LeftStickY=0;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Down) {
                LeftStickY=63;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Fire1) {
                LeftButton1Status=1;
                ReturnValue=0;
            }
            if (TempCode==LeftJS.Fire2) {
                LeftButton2Status=1;
                ReturnValue=0;
            }
        }

        if (RightJS.UseMouse==0) {
            if (TempCode==RightJS.Left) {
                ReturnValue=0;
                RightStickX=0;
            }
            if (TempCode==RightJS.Right) {
                RightStickX=63;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Up) {
                RightStickY=0;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Down) {
                RightStickY=63;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Fire1) {
                RightButton1Status=1;
                ReturnValue=0;
            }
            if (TempCode==RightJS.Fire2) {
                RightButton2Status=1;
                ReturnValue=0;
            }
        }
        break;
    }
    return(ReturnValue);
}

/****************************************************************************/
// Called by vcc.c to set mouse joystick buttons
// Side=0 Left Button Side=1 Right Button State 1=Down

void
SetButtonStatus(unsigned char Side,unsigned char State)
{
    unsigned char Btemp;
    Btemp = (Side<<1) | State;
    if (LeftJS.UseMouse==1)
        switch (Btemp) {
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

    if (RightJS.UseMouse==1)
        switch (Btemp) {
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

